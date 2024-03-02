//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[7] = {"Static", "Bimodal", "Gshare", "Local", "Tournament", "Perceptron"};

int ghistoryBits; // Indicates the length of Global History kept
int lhistoryBits; // Indicates the length of Local History kept in the PHT
int pcIndexBits;  // Indicates the number of bits used to index the PHT
int bpType;       // Branch Prediction Type
int verbose;

int bimodalBits;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//
uint8_t *PHT;
uint64_t GHR;
uint8_t *CPHT;
uint8_t *BHT;
uint8_t *GPHT;
uint8_t *LPHT;



//------------------------------------//
//        Helper Functions            //
//------------------------------------//
uint32_t mask_data(uint32_t data, int bits)
{
  return data & ((1 << bits) - 1);
}

//------------------------------------//
//               BiModal              //
//------------------------------------//

void init_bimodal()
{
  int PHT_size = 1 << bimodalBits;
  PHT = (uint8_t *)malloc(PHT_size * sizeof(uint8_t));

  for (int i = 0; i < PHT_size; i++)
  {
    PHT[i] = WN;
  }
}

uint8_t bimodal_prediction(uint32_t pc)
{
  int index = pc % (1 << bimodalBits);
  return PHT[index] >= WT ? TAKEN : NOTTAKEN;
}

void train_bimodal(uint32_t pc, uint8_t outcome)
{
  int index = pc % (1 << bimodalBits);
  if (outcome == TAKEN && PHT[index] < ST)
  {
    PHT[index]++;
  }
  else if (outcome == NOTTAKEN && PHT[index] > SN)
  {
    PHT[index]--;
  }
}

void bimodal_clean()
{
  free(PHT);
}

//------------------------------------//
//               Gshare               //
//------------------------------------//
void init_gshare()
{
  int GPHT_size = 1 << ghistoryBits;
  GPHT = (uint8_t *)malloc(GPHT_size * sizeof(uint8_t));
  for (int i = 0; i < GPHT_size; i++)
  {
    GPHT[i] = WN;
  }
  GHR = 0;
}

int gshare_index(uint32_t pc) {
  return mask_data(pc, ghistoryBits) ^ mask_data(GHR, ghistoryBits);
}

uint8_t gshare_prediction(uint32_t pc)
{
  int index = gshare_index(pc);
  return GPHT[index] >= WT ? TAKEN : NOTTAKEN;
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
  int index = gshare_index(pc);
  if (outcome == TAKEN && GPHT[index] < ST)
  {
    GPHT[index]++;
  }
  else if (outcome == NOTTAKEN && GPHT[index] > SN)
  {
    GPHT[index]--;
  }
  GHR = (GHR << 1) | outcome;
}

void clean_gshare()
{
  free(GPHT);
}

//------------------------------------//
//           local                    //
//------------------------------------//
void init_local()
{
  int BHT_size = 1 << lhistoryBits;
  BHT = (uint8_t *)malloc(BHT_size * sizeof(uint8_t));
  for (int i = 0; i < BHT_size; i++)
  {
    BHT[i] = 0;
  }

  int LPHT_size = 1 << pcIndexBits;
  LPHT = (uint8_t *)malloc(LPHT_size * sizeof(uint8_t));
  for (int i = 0; i < LPHT_size; i++)
  {
    LPHT[i] = WN;
  }
}

int local_index(uint32_t pc)
{
  return BHT[mask_data(pc, lhistoryBits)] ^ mask_data(pc, pcIndexBits);
}

void train_local(uint32_t pc, uint8_t outcome)
{
  int index = local_index(pc);
  if (outcome == TAKEN && LPHT[index] < ST)
  {
    LPHT[index]++;
  }
  else if (outcome == NOTTAKEN && LPHT[index] > SN)
  {
    LPHT[index]--;
  }

  int bht_index = mask_data(pc, lhistoryBits);
  BHT[bht_index] = (BHT[bht_index] << 1) | outcome;
}

uint8_t local_prediction(uint32_t pc)
{
  int lpht_idx = local_index(pc);
  uint8_t local_res = LPHT[lpht_idx] >= WT ? TAKEN : NOTTAKEN;
  return local_res;
}

void clean_local()
{
  free(BHT);
  free(LPHT);
}

//------------------------------------//
//           tournament               //
//------------------------------------//
void init_tournament()
{

  int CPHT_size = 1 << ghistoryBits;
  CPHT = (uint8_t *)malloc(CPHT_size * sizeof(uint8_t));
  for (int i = 0; i < CPHT_size; i++)
  {
    CPHT[i] = WN;
  }

  init_gshare();
  init_local();
}

int cpht_index(uint32_t pc)
{
  return mask_data(pc>>3, ghistoryBits) ^ mask_data(GHR, ghistoryBits);
}

uint8_t cpht_selector(uint32_t pc)
{
  int index = cpht_index(pc);
  uint8_t selector = CPHT[index] >= WT ? TAKEN : NOTTAKEN;
  return selector;
}

uint8_t tournament_prediction(uint32_t pc)
{
  uint8_t gshare_res = gshare_prediction(pc);
  uint8_t local_res = local_prediction(pc);
  uint8_t selector = cpht_selector(pc);
  if (selector == TAKEN) // 2 3 use gshare; 0 1 use local
    return gshare_res;
  return local_res;
}

void train_tournament(uint32_t pc, uint8_t outcome)
{
  uint8_t gshare_res = gshare_prediction(pc);
  uint8_t local_res = local_prediction(pc);
  if (gshare_res != local_res)
  {
    int index = cpht_index(pc);
    if (gshare_res == outcome)
    {
      if (CPHT[index] < ST)
        CPHT[index]++;
    }
    else
    {
      if (CPHT[index] > SN)
        CPHT[index]--;
    }
  }
  train_gshare(pc, outcome);
  train_local(pc, outcome);
}

void clean_tournament()
{
  free(CPHT);
  free(BHT);
  free(GPHT);
  free(LPHT);
}

//------------------------------------//
//             perceptron             //
//------------------------------------//
#include<math.h>

// ref https://cseweb.ucsd.edu/classes/wi10/cse240a/Slides/11_BranchPapers.pdf

#define HIST_LEN   59       // Optimal value as determined by paper
// #define TABLE_SIZE 1024  // Determined by threshold and history length
#define THRESHOLD  127       // floor(1.93 * HIST_LEN + 14)   // Floor(1.93 * HIST_LEN + 14) as specified in paper

int pTableBits;         // Determined by threshold and history length
int** pTable;       // Perceptron table
int y_abs;
int y_sign;

uint32_t perceptron_index(uint32_t pc) {
  return mask_data(pc>>2, pTableBits) ^ mask_data(GHR, pTableBits);
}

void init_perceptron()
{
  GHR = 0;
  int ptable_size = 1 << pTableBits;
  pTable = (int **)malloc(ptable_size * sizeof(int*));
  for (int i = 0; i < ptable_size; i++)
  {
    pTable[i] = (int *)malloc(HIST_LEN * sizeof(int));
    for (int j = 0; j < HIST_LEN; j++)
    {
      pTable[i][j] = 1;
    }
  }

}

int  perceptron_accu(uint32_t pc) {
  int index = perceptron_index(pc);
  int y = pTable[index][0];  // w0
  for (int i = 1; i <= HIST_LEN; i++)
  { 
    int GHRi = ((GHR >> i) & 1) ? 1 : -1;

    y += GHRi * pTable[index][i];

   // printf("GHRi: %d, pi: %d, y: %d\n", GHRi, pTable[index][i], y);
  }
 
  y_abs   = abs(y);
  y_sign  = y >= 0 ? TAKEN : NOTTAKEN;
  return y;
}

uint8_t perceptron_prediction(uint32_t pc)
{
  int y = perceptron_accu(pc);
  if (y >= 0) return TAKEN;  
  return NOTTAKEN;
}

void train_perceptron(uint32_t pc, uint8_t outcome)
{
  int index = perceptron_index(pc);
  if ( (y_sign != outcome) || (y_abs <= THRESHOLD)) {

    // update bias, if outcome match, ++ bias, else -- bias
    if (outcome==TAKEN) {
      pTable[index][0]++;
      if (pTable[index][0] > THRESHOLD) pTable[index][0] = THRESHOLD;
    } else {
      pTable[index][0]--;
      if (pTable[index][0] < (-1 * THRESHOLD)) pTable[index][0] = (-1*THRESHOLD);
    }
   
    // update weight: if branch outcome match GHR, ++ weight, else -- weight
    for (int i = 1; i <= HIST_LEN; i++) {
      int GHRi = (GHR >> i) & 1;
  
      if (outcome == GHRi) {
        pTable[index][i]++;
        if (pTable[index][i] > THRESHOLD) pTable[index][i] = THRESHOLD;
      } else {
        pTable[index][i]--;
        if (pTable[index][i] < -THRESHOLD) pTable[index][i] = -THRESHOLD;
      }
    }
  }

  GHR = (GHR << 1) | outcome;
}

void clean_perceptron()
{
  int ptable_size = 1 << pTableBits;
  for (int i = 0; i < ptable_size; i++)
  {
    free(pTable[i]);
  }
  free(pTable);
}


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case BIMODAL:
    init_bimodal();
    break;
  case GSHARE:
    init_gshare();
    break;
  case LOCAL:
    init_local();
    break;
  case TOURNAMENT:
    init_tournament();
    break;
  case PERCEPTRON:
    init_perceptron();
    break;
  default:
    printf("init bpType = %dnot found\n", bpType);
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  uint8_t res = NOTTAKEN;
  //  Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    res = TAKEN;
    break;
  case BIMODAL:
    res = bimodal_prediction(pc);
    break;
  case GSHARE:
    res = gshare_prediction(pc);
    break;
  case LOCAL:
    res = local_prediction(pc);
    break;
  case TOURNAMENT:
    res = tournament_prediction(pc);
    break;
  case PERCEPTRON:
    res = perceptron_prediction(pc);
    break;
  default:
    printf("make bpType = %d not found\n", bpType);
    break;
  }

  return res;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void train_predictor(uint32_t pc, uint8_t outcome)
{
  switch (bpType)
  {
  case STATIC:
    break;
  case BIMODAL:
    train_bimodal(pc, outcome);
    break;
  case GSHARE:
    train_gshare(pc, outcome);
    break;
  case LOCAL:
    train_local(pc, outcome);
    break;
  case TOURNAMENT:
    train_tournament(pc, outcome);
    break;
  case PERCEPTRON:
    train_perceptron(pc, outcome);
    break;
  default:
    printf("train bpType = %d not found\n", bpType);
    break;
  }
}

void clean()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case BIMODAL:
    bimodal_clean();
    break;
  case GSHARE:
    clean_gshare();
    break;
  case LOCAL:
    clean_local();
    break;
  case TOURNAMENT:
    clean_tournament();
    break;
  case PERCEPTRON:
    clean_perceptron();
    break;
  default:
    break;
  }
}