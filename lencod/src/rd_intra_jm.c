/*!
 ***************************************************************************
 * \file rd_intra_jm.c
 *
 * \brief
 *    Rate-Distortion optimized mode decision
 *
 * \author
 *    - Heiko Schwarz              <hschwarz@hhi.de>
 *    - Valeri George              <george@hhi.de>
 *    - Lowell Winger              <lwinger@lsil.com>
 *    - Alexis Michael Tourapis    <alexismt@ieee.org>
 * \date
 *    12. April 2001
 **************************************************************************
 */

#include <limits.h>

#include "global.h"

#include "image.h"
#include "macroblock.h"
#include "mb_access.h"
#include "rdopt_coding_state.h"
#include "mode_decision.h"
#include "rdopt.h"
#include "rd_intra_jm.h"
#include "q_around.h"
#include "intra4x4.h"
#include "intra8x8.h"
#include "md_common.h"
#include "transform8x8.h"
#include "md_distortion.h"
#include "elements.h"
#include "symbol.h"

extern int MBType2Value (Macroblock* currMB);

void dct4x4_daniel(int **block)
{
	int a[16],b[16],c[16];
	
	a[0] = block[0][0] + block[3][0];
    a[1] = block[1][0] + block[2][0];
    a[2] = block[0][1] + block[3][1];
    a[3] = block[1][1] + block[2][1];
    a[4] = block[0][2] + block[3][2];
    a[5] = block[1][2] + block[2][2];
    a[6] = block[0][3] + block[3][3];
    a[7] = block[1][3] + block[2][3];
    a[8] = block[0][0] - block[3][0];
    a[9] = block[1][0] - block[2][0];
    a[10] = block[0][1] - block[3][1];
    a[11] = block[1][1] - block[2][1];
    a[12] = block[0][2] - block[3][2];
    a[13] = block[1][2] - block[2][2];
    a[14] = block[0][3] - block[3][3];
    a[15] = block[1][3] - block[2][3];
                            
    b[0] = a[0] + a[1];
    b[1] = a[2] + a[3];
    b[2] = a[4] + a[5];
    b[3] = a[6] + a[7];
	b[4] = a[8] + a[14];
	b[5] = a[9] + a[15];
	b[6] = a[10] + a[12];
	b[7] = a[11] + a[13];
	b[8] = a[8] - a[14];
	b[9] = a[9] - a[15];
	b[10] = a[10] - a[12];
	b[11] = a[11] - a[13];	
	b[12] = a[0] - a[1];
    b[13] = a[2] - a[3];
    b[14] = a[4] - a[5];
    b[15] = a[6] - a[7];

	c[0] = b[0] + b[3];
	c[1] = b[1] + b[2];
	c[2] = b[0] - b[3];
	c[3] = b[1] - b[2];
	c[4] = b[4] + b[6];
	c[5] = b[5] + b[7];
	c[6] = b[4] - b[6];
	c[7] = b[5] - b[7];
	c[8] = 2*b[8] + b[9];
	c[9] = 2*b[10] + b[11];
	c[10] = b[8] - 2*b[9];
	c[11] = b[10] - 2*b[11];
	c[12] = b[12] + b[15];
	c[13] = b[13] + b[14];
	c[14] = b[12] - b[15];
	c[15] = b[13] - b[14];
	
	block[0][0] = c[0] + c[1];
	block[0][1] = 2*c[2] + c[3];
	block[0][2] = c[0] - c[1];
	block[0][3] = c[2] - 2*c[3];
	block[1][0] = 2*c[4] + c[5];
	block[1][1] = 2*c[8] + c[9];
	block[1][2] = 2*c[6] + c[7];
	block[1][3] = c[8] - 2*c[9];
	block[2][0] = c[12] + c[13];
	block[2][1] = 2*c[14] + c[15];
	block[2][2] = c[12] - c[13];
	block[2][3] = c[14] - 2*c[15];
	block[3][0] = c[4] - 2*c[5];
	block[3][1] = 2*c[10] + c[11];
	block[3][2] = c[6] - 2*c[7];
	block[3][3] = c[10] - 2*c[11];
}             

void hadamard4x4_daniel(int **block)
{
	int a[16],b[16],c[16];
	
	a[0] = block[0][0] + block[1][0];
	a[1] = block[2][0] + block[3][0];
	a[2] = block[0][1] + block[1][1];
	a[3] = block[2][1] + block[3][1];
	a[4] = block[0][2] + block[1][2];
	a[5] = block[2][2] + block[3][2];
	a[6] = block[0][3] + block[1][3];
    a[7] = block[2][3] + block[3][3];
	a[8] = block[0][0] - block[1][0];
	a[9] = block[2][0] - block[3][0];
	a[10] = block[0][1] - block[1][1];
	a[11] = block[2][1] - block[3][1];
	a[12] = block[0][2] - block[1][2];
	a[13] = block[2][2] - block[3][2];
	a[14] = block[0][3] - block[1][3];
	a[15] = block[2][3] - block[3][3];
	
	b[0] = a[0] + a[1];
	b[1] = a[2] + a[3];
	b[2] = a[4] + a[5];
	b[3] = a[6] + a[7];
	b[4] = a[0] - a[1];
	b[5] = a[2] - a[3];
	b[6] = a[4] - a[5];
    b[7] = a[6] - a[7];
	b[8] = a[8] - a[9];
	b[9] = a[10] - a[11];
	b[10] = a[12] - a[13];
	b[11] = a[14] - a[15];
	b[12] = a[8] + a[9];
	b[13] = a[10] + a[11];
	b[14] = a[12] + a[13];
	b[15] = a[14] + a[15];
	
	c[0] = b[0] + b[1];
	c[1] = b[2] + b[3];
	c[2] = b[0] - b[1];
	c[3] = b[2] - b[3];
	c[4] = b[4] + b[5];
	c[5] = b[6] + b[7];
	c[6] = b[4] - b[5];
    c[7] = b[6] - b[7];
	c[8] = b[8] + b[9];
	c[9] = b[10] + b[11];
	c[10] = b[8] - b[9];
	c[11] = b[10] - b[11];
	c[12] = b[12] + b[13];
	c[13] = b[14] + b[15];
	c[14] = b[12] - b[13];
	c[15] = b[14] - b[15];
	
	block[0][0] = (c[0] + c[1])/2;
	block[0][1] = (c[0] - c[1])/2;
	block[0][2] = (c[2] - c[3])/2;
	block[0][3] = (c[2] + c[3])/2;
	block[1][0] = (c[4] + c[5])/2;
	block[1][1] = (c[4] - c[5])/2;
	block[1][2] = (c[6] - c[4])/2;
	block[1][3] = (c[6] + c[7])/2;
	block[2][0] = (c[8] + c[9])/2;
	block[2][1] = (c[8] - c[9])/2;
	block[2][2] = (c[10] - c[11])/2;
	block[2][3] = (c[10] + c[11])/2;
	block[3][0] = (c[12] + c[13])/2;
	block[3][1] = (c[12] - c[13])/2;
	block[3][2] = (c[14] - c[15])/2;
	block[3][3] = (c[14] + c[15])/2;
}

void quantization_daniel(int **block)
{
	int QP = 28;
	int MF[6][4][4] = {	{	{13107,8066,13107,8066},
							{8066, 5243,8066, 5243},
							{13107,8066,13107,8066},
							{8066, 5243,8066, 5243}},

						{	{11916,7490,11916,7490},
							{7490, 4660,7490, 4660},
							{11916,7490,11916,7490},
							{7490, 4660,7490, 4660}},

						{	{10082,6554,10082,6554},
							{6554, 4194,6554, 4194},
							{10082,6554,10082,6554},
							{6554, 4194,6554, 4194}},

						{	{9362,5825,9362,5825},
							{5825, 3647,5825, 3647},
							{9362,5825,9362,5825},
							{5825, 3647,5825, 3647}},

						{	{8192,5243,8192,5243},
							{5243, 3355,5243, 3355},
							{8192,5243,8192,5243},
							{5243, 3355,5243, 3355}},

						{	{7282,4559,7282,4559},
							{4559, 2893,4559, 2893},
							{7282,4559,7282,4559},
							{4559, 2893,4559, 2893}}};

	int f[6] = {10912,21824,43648,87296,174592,349184};

	int qbits = 15 + QP/6;

	int i,j;
	for(i=0;i<4;i++)
		for(j=0;j<4;j++)
			block[i][j] = ( abs(block[i][j])*MF[QP/6][i][j] + f[QP%6] ) >> qbits;
}


int satd_dct(int **block)
{
	int i,j;

	int distortion=0;
	
	dct4x4_daniel(block);
	quantization_daniel(block);
	
	for(i=0;i<4;i++)
		for(j=0;j<4;j++)
			distortion += abs(block[i][j]); 
	return distortion;
}

int satd(int **block)
{
	int i,j;
	int erroSatd =0;
	hadamard4x4_daniel(block);
	for(i=0;i<4;i++)
		for(j=0;j<4;j++)
			erroSatd += abs(block[i][j]);
	return erroSatd;
}

////Fun��o nova para calcular rdcost

int novo_rdcost(imgpel **cur_img, imgpel **prd_img, imgpel **cur_prd, 
                         int **m7, int pic_opix_x, int block_x)
{
  int **block;

  int j, i, *m7_line,aux,sad,sse;
  imgpel *cur_line, *prd_line;

  block = (int**) malloc(sizeof(int*)*4);			//
  for (i=0; i<4;i++)							//Aloca��o do bloco 4x4
	block[i] = (int*) malloc(sizeof(int)*4);	//

  sad=0;
  sse=0;
  for (j = 0; j < BLOCK_SIZE; j++)
  {
    m7_line = &m7[j][block_x];
    cur_line = &cur_img[j][pic_opix_x];
    prd_line = prd_img[j];
    memcpy(&cur_prd[j][block_x], prd_line, BLOCK_SIZE * sizeof(imgpel));

    for (i = 0; i < BLOCK_SIZE; i++)
    {
      aux = *cur_line - *prd_line;
	  block[j][i]=aux;

	  *m7_line++ = (int) (*cur_line++ - *prd_line++);	  
	  sad+= abs(aux);
	  sse+= iabs2(aux);
    }
	
  } 

  return satd_dct(block);
}





/*!
 *************************************************************************************
 * \brief
 *    Mode Decision for an 4x4 Intra block
 *************************************************************************************
 */

int Mode_Decision_for_4x4IntraBlocks_JM_High (Macroblock *currMB, int  b8,  int  b4,  int  lambda,  distblk*  min_cost)
{
  VideoParameters *p_Vid = currMB->p_Vid;
  InputParameters *p_Inp = currMB->p_Inp;
  Slice *currSlice = currMB->p_slice;
  RDOPTStructure  *p_RDO = currSlice->p_RDO;

  int     ipmode, best_ipmode = 0, y;
  int     available_mode;
  int     c_nz, nonzero = 0;
  int*    ACLevel = currSlice->cofAC[b8][b4][0];
  int*    ACRun   = currSlice->cofAC[b8][b4][1];
  distblk rdcost = 0;
  distblk min_rdcost  = DISTBLK_MAX;
  int    block_x     = ((b8 & 0x01) << 3) + ((b4 & 0x01) << 2);
  int    block_y     = ((b8 >> 1) << 3)  + ((b4 >> 1) << 2);
  int    pic_pix_x   = currMB->pix_x  + block_x;
  int    pic_pix_y   = currMB->pix_y  + block_y;
  int    pic_opix_x  = currMB->pix_x + block_x;
  int    pic_opix_y  = currMB->opix_y + block_y;
  int    pic_block_x = pic_pix_x >> 2;
  int    pic_block_y = pic_pix_y >> 2;

  int left_available, up_available, all_available;
  int *mb_size = p_Vid->mb_size[IS_LUMA];

  char   upMode, leftMode;
  int    mostProbableMode;

  PixelPos left_block, top_block;

  int  lrec4x4[4][4];
  int best_nz_coeff = 0;
  int block_x4 = block_x>>2;
  int block_y4 = block_y>>2;
  //int taxaBits;

#ifdef BEST_NZ_COEFF
  int best_coded_block_flag = 0;
  int bit_pos = 1 + ((((b8>>1)<<1)+(b4>>1))<<2) + (((b8&1)<<1)+(b4&1));
  int64 cbp_bits;

  if (b8==0 && b4==0)
    cbp_bits = 0;  
#endif

  get4x4Neighbour(currMB, block_x - 1, block_y    , mb_size, &left_block);
  get4x4Neighbour(currMB, block_x,     block_y - 1, mb_size, &top_block );

  // constrained intra pred
  if (p_Inp->UseConstrainedIntraPred)
  {
    left_block.available = left_block.available ? p_Vid->intra_block[left_block.mb_addr] : 0;
    top_block.available  = top_block.available  ? p_Vid->intra_block[top_block.mb_addr]  : 0;
  }

  upMode            =  top_block.available ? p_Vid->ipredmode[top_block.pos_y ][top_block.pos_x ] : (char) -1;
  leftMode          = left_block.available ? p_Vid->ipredmode[left_block.pos_y][left_block.pos_x] : (char) -1;

  mostProbableMode  = (upMode < 0 || leftMode < 0) ? DC_PRED : upMode < leftMode ? upMode : leftMode;
  *min_cost = DISTBLK_MAX;

  currMB->ipmode_DPCM = NO_INTRA_PMODE; ////For residual DPCM

  //===== INTRA PREDICTION FOR 4x4 BLOCK =====
  // set intra prediction values for 4x4 intra prediction
  set_intrapred_4x4(currMB, PLANE_Y, pic_pix_x, pic_pix_y, &left_available, &up_available, &all_available);  

  
  

  //===== LOOP OVER ALL 4x4 INTRA PREDICTION MODES =====
  for (ipmode = 0; ipmode < NO_INTRA_PMODE; ipmode++)
  {

    available_mode =  (all_available) || (ipmode==DC_PRED) ||
      (up_available && (ipmode==VERT_PRED||ipmode==VERT_LEFT_PRED||ipmode==DIAG_DOWN_LEFT_PRED)) ||
      (left_available && (ipmode==HOR_PRED||ipmode==HOR_UP_PRED));

    if (valid_intra_mode(currSlice, ipmode) == 0)
      continue;

    //if( available_mode)
	if(1)
    {
      // generate intra 4x4 prediction block given availability
      get_intrapred_4x4(currMB, PLANE_Y, ipmode, block_x, block_y, left_available, up_available);

      // get prediction and prediction error
      generate_pred_error_4x4(&p_Vid->pCurImg[pic_opix_y], currSlice->mpr_4x4[0][ipmode], &currSlice->mb_pred[0][block_y], &currSlice->mb_ores[0][block_y], pic_opix_x, block_x);     

      // get and check rate-distortion cost
#ifdef BEST_NZ_COEFF
      currMB->cbp_bits[0] = cbp_bits;
#endif	
	  rdcost = currSlice->rdcost_for_4x4_intra_blocks (currMB, &c_nz, b8, b4, ipmode, lambda, mostProbableMode, min_rdcost);
	  
	  //rdcost = novo_rdcost(&p_Vid->pCurImg[pic_opix_y], currSlice->mpr_4x4[0][ipmode], &currSlice->mb_pred[0][block_y], &currSlice->mb_ores[0][block_y], pic_opix_x, block_x);
      
	  if ((rdcost < min_rdcost) || (rdcost == min_rdcost && ipmode == mostProbableMode))
      {
        //--- set coefficients ---
        memcpy(p_RDO->cofAC4x4[0], ACLevel, 18 * sizeof(int));
        memcpy(p_RDO->cofAC4x4[1], ACRun,   18 * sizeof(int));

        //--- set reconstruction ---
        copy_4x4block(p_RDO->rec4x4[PLANE_Y], &p_Vid->enc_picture->imgY[pic_pix_y], 0, pic_pix_x);

        // SP/SI reconstruction
        if(currSlice->slice_type == SP_SLICE && !currSlice->sp2_frame_indicator)
        {
          for (y=0; y<4; y++)
          {
            memcpy(lrec4x4[y],&p_Vid->lrec[pic_pix_y+y][pic_pix_x], BLOCK_SIZE * sizeof(int));// stores the mode coefficients
          }
        }

        //--- flag if dct-coefficients must be coded ---
        nonzero = c_nz;

        //--- set best mode update minimum cost ---
        *min_cost     = rdcost;
        min_rdcost    = rdcost;
        best_ipmode   = ipmode;

        best_nz_coeff = p_Vid->nz_coeff [p_Vid->current_mb_nr][block_x4][block_y4];
#ifdef BEST_NZ_COEFF
        best_coded_block_flag = (int)((currMB->cbp_bits[0] >> bit_pos)&(int64)(1));
#endif
        if (p_Vid->AdaptiveRounding)
        {
          store_adaptive_rounding_4x4 (p_Vid, p_Vid->ARCofAdj4x4, I4MB, block_y, block_x);
        }

      }
    }
  }
#if INTRA_RDCOSTCALC_NNZ
  p_Vid->nz_coeff [p_Vid->current_mb_nr][block_x4][block_y4] = best_nz_coeff;
#endif
#ifdef BEST_NZ_COEFF
  cbp_bits &= (~(int64)(1<<bit_pos));
  cbp_bits |= (int64)(best_coded_block_flag<<bit_pos);
#endif

  //===== set intra mode prediction =====
  p_Vid->ipredmode[pic_block_y][pic_block_x] = (char) best_ipmode;
  currMB->intra_pred_modes[4*b8+b4] =
    (char) (mostProbableMode == best_ipmode ? -1 : (best_ipmode < mostProbableMode ? best_ipmode : best_ipmode-1)); 

  //===== restore coefficients =====
  memcpy (ACLevel, p_RDO->cofAC4x4[0], 18 * sizeof(int));
  memcpy (ACRun,   p_RDO->cofAC4x4[1], 18 * sizeof(int));

  //===== restore reconstruction and prediction (needed if single coeffs are removed) =====
  copy_4x4block(&p_Vid->enc_picture->imgY[pic_pix_y], p_RDO->rec4x4[PLANE_Y], pic_pix_x, 0);
  copy_4x4block(&currSlice->mb_pred[0][block_y], currSlice->mpr_4x4[0][best_ipmode], block_x, 0);

  // SP/SI reconstuction
  if(currSlice->slice_type == SP_SLICE && !currSlice->sp2_frame_indicator)
  {
    for (y=0; y<BLOCK_SIZE; y++)
    {
      memcpy (&p_Vid->lrec[pic_pix_y+y][pic_pix_x], lrec4x4[y], BLOCK_SIZE * sizeof(int));//restore coefficients when encoding primary SP frame
    }
  }

  if (p_Vid->AdaptiveRounding)
  {
    update_adaptive_rounding_4x4 (p_Vid,p_Vid->ARCofAdj4x4, I4MB, block_y, block_x);
  }


  return nonzero;
}

/*!
*************************************************************************************
* \brief
*    8x8 Intra mode decision for a macroblock - High complexity
*************************************************************************************
*/
int Mode_Decision_for_8x8IntraBlocks_JM_High (Macroblock *currMB, int b8, int lambda, distblk *min_cost)
{
  VideoParameters *p_Vid = currMB->p_Vid;
  InputParameters *p_Inp = currMB->p_Inp;
  Slice *currSlice = currMB->p_slice;
  RDOPTStructure  *p_RDO = currSlice->p_RDO;

  int     ipmode, best_ipmode = 0, j;
  int     c_nz, nonzero = 0;  
  distblk  rdcost = 0;
  distblk  min_rdcost  = DISTBLK_MAX;
  int     block_x     = (b8 & 0x01) << 3;
  int     block_y     = (b8 >> 1) << 3;
  int     pic_pix_x   = currMB->pix_x + block_x;
  int     pic_pix_y   = currMB->pix_y + block_y;
  int     pic_opix_x  = currMB->pix_x + block_x;
  int     pic_opix_y  = currMB->opix_y + block_y;
  int     pic_block_x = pic_pix_x >> 2;
  int     pic_block_y = pic_pix_y >> 2;
  int     mb_block_y  = (currMB->block_y) + ((b8 >> 1) << 1);
  int     mb_block_x  = (currMB->block_x) + ((b8 & 0x01) << 1);
  int     *p_AC8x8 = p_RDO->coefAC8x8intra[b8][0][0][0];
  int     *cofAC   = currSlice->cofAC[b8][0][0];

  int    left_available, up_available, all_available;
  int    **mb_ores = currSlice->mb_ores[0]; 
  imgpel **mb_pred = currSlice->mb_pred[0];

  char   upMode, leftMode;
  int    mostProbableMode;

  PixelPos left_block, top_block;

  int *mb_size = p_Vid->mb_size[IS_LUMA];

  get4x4Neighbour(currMB, block_x - 1, block_y    , mb_size, &left_block);
  get4x4Neighbour(currMB, block_x,     block_y - 1, mb_size, &top_block );

  // constrained intra pred
  if (p_Inp->UseConstrainedIntraPred)
  {
    left_block.available = left_block.available ? p_Vid->intra_block [left_block.mb_addr] : 0;
    top_block.available  = top_block.available  ? p_Vid->intra_block [top_block.mb_addr ] : 0;
  }

  if(b8 >> 1)
    upMode    =  top_block.available ? p_Vid->ipredmode8x8[top_block.pos_y ][top_block.pos_x ] : (char) -1;
  else
    upMode    =  top_block.available ? p_Vid->ipredmode   [top_block.pos_y ][top_block.pos_x ] : (char) -1;

  if(b8 & 0x01)
    leftMode  = left_block.available ? p_Vid->ipredmode8x8[left_block.pos_y][left_block.pos_x] : (char) -1;
  else
    leftMode  = left_block.available ? p_Vid->ipredmode[left_block.pos_y][left_block.pos_x] : (char) -1;

  mostProbableMode  = (upMode < 0 || leftMode < 0) ? DC_PRED : upMode < leftMode ? upMode : leftMode;
  *min_cost = DISTBLK_MAX;
  currMB->ipmode_DPCM = NO_INTRA_PMODE; //For residual DPCM

  //===== INTRA PREDICTION FOR 8x8 BLOCK =====
  set_intrapred_8x8(currMB, PLANE_Y, pic_pix_x, pic_pix_y, &left_available, &up_available, &all_available);

  //===== LOOP OVER ALL 8x8 INTRA PREDICTION MODES =====
  for (ipmode = 0; ipmode < NO_INTRA_PMODE; ipmode++)
  {
    if( (ipmode==DC_PRED) ||
      ((ipmode==VERT_PRED||ipmode==VERT_LEFT_PRED||ipmode==DIAG_DOWN_LEFT_PRED) && up_available ) ||
      ((ipmode==HOR_PRED||ipmode==HOR_UP_PRED) && left_available ) ||
      (all_available) )
    {
      get_intrapred_8x8(currMB, PLANE_Y, ipmode, left_available, up_available);
      // get prediction and prediction error
      generate_pred_error_8x8(&p_Vid->pCurImg[pic_opix_y], currSlice->mpr_8x8[0][ipmode], &mb_pred[block_y], &mb_ores[block_y], pic_opix_x, block_x);     

      currMB->ipmode_DPCM = (short) ipmode;

      // get and check rate-distortion cost

      rdcost = currSlice->rdcost_for_8x8_intra_blocks (currMB, &c_nz, b8, ipmode, lambda, min_rdcost, mostProbableMode);
      if ((rdcost < min_rdcost) || (rdcost == min_rdcost && ipmode == mostProbableMode))
      {
        //--- set coefficients ---
        memcpy(p_AC8x8, cofAC, 4 * 2 * 65 * sizeof(int));

        //--- set reconstruction ---
        copy_image_data_8x8(p_RDO->rec8x8[PLANE_Y], &p_Vid->enc_picture->imgY[pic_pix_y], 0, pic_pix_x);

        if (p_Vid->AdaptiveRounding)
        {
          for (j = block_y; j < block_y + 8; j++)
            memcpy(&p_Vid->ARCofAdj8x8[0][DUMMY][j][block_x],&p_Vid->ARCofAdj8x8[0][I8MB][j][block_x], 8 * sizeof(int));
        }

        //--- flag if dct-coefficients must be coded ---
        nonzero = c_nz;

        //--- set best mode update minimum cost ---
        *min_cost   = rdcost;
        min_rdcost  = rdcost;
        best_ipmode = ipmode;
      }      
    }
  }

  //===== set intra mode prediction =====
  p_Vid->ipredmode8x8[pic_block_y][pic_block_x] = (char) best_ipmode;
  currMB->ipmode_DPCM = (short) best_ipmode; //For residual DPCM

  currMB->intra_pred_modes8x8[4*b8] = (char) ((mostProbableMode == best_ipmode)
    ? -1
    : (best_ipmode < mostProbableMode ? best_ipmode : best_ipmode - 1));

  memset(&p_Vid->ipredmode8x8[mb_block_y    ][mb_block_x], best_ipmode, 2 * sizeof(char));
  memset(&p_Vid->ipredmode8x8[mb_block_y + 1][mb_block_x], best_ipmode, 2 * sizeof(char));

  //===== restore coefficients =====
  memcpy(cofAC, p_AC8x8, 4 * 2 * 65 * sizeof(int));

  if (p_Vid->AdaptiveRounding)
  {
    for (j=block_y; j< block_y + 8; j++)
      memcpy(&p_Vid->ARCofAdj8x8[0][I8MB][j][block_x], &p_Vid->ARCofAdj8x8[0][DUMMY][j][block_x], 8 * sizeof(int));
  }

  //===== restore reconstruction and prediction (needed if single coeffs are removed) =====
  copy_image_data_8x8(&p_Vid->enc_picture->imgY[pic_pix_y], p_RDO->rec8x8[0], pic_pix_x, 0);
  copy_image_data_8x8(&mb_pred[block_y], currSlice->mpr_8x8[0][best_ipmode], block_x, 0);

  return nonzero;
}

/*!
 *************************************************************************************
 * \brief
 *    Mode Decision for an 8x8 Intra block
 *************************************************************************************
 */
int Mode_Decision_for_IntraSubMBlocks(Macroblock *currMB, int b8, int lambda, distblk *cost, int non_zero[3])
{
  Slice *currSlice = currMB->p_slice;
  int  b4;
  distblk  cost4x4;
  currMB->cr_cbp[0] = 0;
  currMB->cr_cbp[1] = 0;
  currMB->cr_cbp[2] = 0;

  memset(non_zero, 0, 3 * sizeof(int));
 *cost = weighted_cost(lambda, 6);        //6 * lambda;
  for (b4=0; b4<4; b4++)
  {
    non_zero[0] |= currSlice->Mode_Decision_for_4x4IntraBlocks (currMB, b8, b4, lambda, &cost4x4);
    non_zero[1] |= currMB->cr_cbp[1];
    non_zero[2] |= currMB->cr_cbp[2];
    *cost += cost4x4;
  }

  return non_zero[0];
}

/*!
 *************************************************************************************
 * \brief
 *    4x4 Intra mode decision for an macroblock
 *************************************************************************************
 */
int Mode_Decision_for_Intra4x4Macroblock (Macroblock *currMB, int lambda,  distblk* cost)
{
  Slice *currSlice = currMB->p_slice;
  int  cbp=0, b8;
  distblk cost8x8;
  int non_zero[3] = {0, 0, 0};

  currSlice->cmp_cbp[1] = currSlice->cmp_cbp[2] = 0;
  
  for (*cost=0, b8=0; b8<4; b8++)
  {
    if (Mode_Decision_for_IntraSubMBlocks (currMB, b8, lambda, &cost8x8, non_zero))
    {
      cbp |= (1<<b8);
    }
    *cost += cost8x8;
    if (non_zero[1])
    {
      currSlice->cmp_cbp[1] |= (1<<b8);
      cbp |= currSlice->cmp_cbp[1];
      currSlice->cmp_cbp[1] = cbp;
      currSlice->cmp_cbp[2] = cbp;
    }
    if (non_zero[2])
    {
      currSlice->cmp_cbp[2] |= (1<<b8);
      cbp |= currSlice->cmp_cbp[2];
      currSlice->cmp_cbp[1] = cbp;
      currSlice->cmp_cbp[2] = cbp;
    }
  }
  return cbp;
}


/*!
*************************************************************************************
* \brief
*    Intra 16x16 mode decision
*************************************************************************************
*/
void Intra16x16_Mode_Decision_SAD (Macroblock* currMB)
{
  Slice *currSlice = currMB->p_slice;
  /* generate intra prediction samples for all 4 16x16 modes */
  intrapred_16x16 (currMB, PLANE_Y);
  currSlice->find_sad_16x16 (currMB);   /* get best new intra mode */
  currMB->cbp = currMB->trans_16x16 (currMB, PLANE_Y);    
}


/*!
 ************************************************************************
 * \brief
 *    Find best Intra_16x16 mode based on min RDCost
 *
 * \par Input:
 *    Image parameters, pointer to best 16x16 intra mode
 *
 * \par Output:
 *    best 16x16 based 
 ************************************************************************/
distblk min_rdcost_16x16(Macroblock *currMB, int lambda)
{
  VideoParameters *p_Vid = currMB->p_Vid;
  InputParameters *p_Inp = currMB->p_Inp; 
  Slice *currSlice = currMB->p_slice;

  SyntaxElement   se;
  const int*      partMap    = assignSE2partition[currSlice->partition_mode];
  DataPartition*  dataPart   = &(currSlice->partArr[partMap[SE_MBTYPE]]);

  distblk min_rdcost = DISTBLK_MAX, rdcost;
  distblk distortionY; 
  int rate; 
  int i,k;
  int b8, b4; 

  PixelPos up;          //!< pixel position p(0,-1)
  PixelPos left[17];    //!< pixel positions p(-1, -1..15)

  int up_avail, left_avail, left_up_avail;

  int  best_mode = 0, best_cbp = 0; 
  int  bestCofAC[4][4][2][65];
  int  bestCofDC[2][18]; 
  imgpel bestRec[MB_BLOCK_SIZE][MB_BLOCK_SIZE];

  for (i=0;i<17;i++)
  {
    p_Vid->getNeighbour(currMB, -1 ,  i - 1 , p_Vid->mb_size[IS_LUMA], &left[i]);
  }

  p_Vid->getNeighbour(currMB, 0     ,  -1 , p_Vid->mb_size[IS_LUMA], &up);

  if (!(p_Inp->UseConstrainedIntraPred))
  {
    up_avail   = up.available;
    left_avail = left[1].available;
    left_up_avail = left[0].available;
  }
  else
  {
    up_avail      = up.available ? p_Vid->intra_block[up.mb_addr] : 0;
    for (i = 1, left_avail = 1; i < 17;i++)
      left_avail  &= left[i].available ? p_Vid->intra_block[left[i].mb_addr]: 0;
    left_up_avail = left[0].available ? p_Vid->intra_block[left[0].mb_addr]: 0;
  }

  currMB->mb_type = I16MB; 

  for (k = 0;k < 4; k++)
  {
    if (p_Inp->IntraDisableInterOnly == 0 || (currSlice->slice_type != I_SLICE && currSlice->slice_type != SI_SLICE))
    {
      if (p_Inp->Intra16x16ParDisable && (k==VERT_PRED_16||k==HOR_PRED_16))
        continue;

      if (p_Inp->Intra16x16PlaneDisable && k==PLANE_16)
        continue;
    }
    if ((k==0 && !up_avail) || (k==1 && !left_avail) || (k==3 && (!left_avail || !up_avail || !left_up_avail)))
      continue; 

    currMB->i16mode = (char) k; 
    currMB->cbp = currMB->trans_16x16(currMB, PLANE_Y); 

    distortionY = compute_SSE16x16(&p_Vid->pCurImg[currMB->opix_y], &p_Vid->enc_picture->p_curr_img[currMB->pix_y], currMB->pix_x, currMB->pix_x);

    currSlice->store_coding_state (currMB, currSlice->p_RDO->cs_tmp);
    currMB->i16offset = I16Offset  (currMB->cbp, currMB->i16mode);

    rate = 0;
    {
      se.value1  = MBType2Value (currMB);
      se.value2  = 0;
      se.type    = SE_MBTYPE;

      currSlice->writeMB_typeInfo (currMB, &se, dataPart);

      rate       += se.len;
    }
    rate += currSlice->writeCoeff16x16   (currMB, PLANE_Y);
    currSlice->reset_coding_state (currMB, currSlice->p_RDO->cs_tmp);

    rdcost = distortionY + weighted_cost(lambda,rate); 

    if(rdcost < min_rdcost)
    {
      min_rdcost = rdcost; 
      best_mode = k; 
      best_cbp = currMB->cbp; 
      for(b8 = 0; b8 < 4; b8++)
      {
        for(b4 = 0; b4 < 4; b4++)
        {
          memcpy(bestCofAC[b8][b4][0], currSlice->cofAC[b8][b4][0], sizeof(int) * 65);
          memcpy(bestCofAC[b8][b4][1], currSlice->cofAC[b8][b4][1], sizeof(int) * 65);
        }
      }

      memcpy(bestCofDC[0], currSlice->cofDC[0][0], sizeof(int)*18);
      memcpy(bestCofDC[1], currSlice->cofDC[0][1], sizeof(int)*18);

      for(i = 0; i < MB_BLOCK_SIZE; i++)
        memcpy(bestRec[i], &p_Vid->enc_picture->p_curr_img[currMB->pix_y + i][currMB->pix_x], sizeof(imgpel)*MB_BLOCK_SIZE);
    }
  }

  currMB->i16mode = (char) best_mode;
  currMB->cbp = best_cbp; 
  currMB->i16offset = I16Offset  (currMB->cbp, currMB->i16mode);

  for(b8 = 0; b8 < 4; b8++)
  {
    for(b4 = 0; b4 < 4; b4++)
    {
      memcpy(currSlice->cofAC[b8][b4][0], bestCofAC[b8][b4][0], sizeof(int)*65);
      memcpy(currSlice->cofAC[b8][b4][1], bestCofAC[b8][b4][1], sizeof(int)*65);
    }
  }

  memcpy(currSlice->cofDC[0][0], bestCofDC[0], sizeof(int)*18);
  memcpy(currSlice->cofDC[0][1], bestCofDC[1], sizeof(int)*18);

  for(i = 0; i < MB_BLOCK_SIZE; i++)
  {
    memcpy(&p_Vid->enc_picture->p_curr_img[currMB->pix_y+i][currMB->pix_x], bestRec[i], MB_BLOCK_SIZE*sizeof(imgpel));
  }

  return min_rdcost;
}



/*!
*************************************************************************************
* \brief
*    Intra 16x16 mode decision using Rate-Distortion Optimization
*************************************************************************************
*/
void Intra16x16_Mode_Decision_RDopt (Macroblock* currMB, int lambda)
{


  /* generate intra prediction samples for all 4 16x16 modes */
  intrapred_16x16 (currMB, PLANE_Y);
  min_rdcost_16x16 (currMB, lambda);   /* get best new intra mode */ 
  
  
}


