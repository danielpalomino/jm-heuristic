
/*!
 *************************************************************************************
 * \file annexb.c
 *
 * \brief
 *    Annex B Byte Stream format NAL Unit writing routines
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *      - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 *************************************************************************************
 */

#include "global.h"
#include "nalucommon.h"

/*!
 ********************************************************************************************
 * \brief
 *    Writes a NALU to the Annex B Byte Stream
 *
 * \return
 *    number of bits written
 *
 ********************************************************************************************
*/
int WriteAnnexbNALU (VideoParameters *p_Vid, NALU_t *n)
{
  int BitsWritten = 0;
  int offset = 0;
  int length = 4;
  static const byte startcode[] = {0,0,0,1};
  byte first_byte;

  assert (n != NULL);
  assert (n->forbidden_bit == 0);
  assert (p_Vid->f_annexb != NULL);
  assert (n->startcodeprefix_len == 3 || n->startcodeprefix_len == 4);

// printf ("WriteAnnexbNALU: writing %d bytes w/ startcode_len %d\n", n->len+1, n->startcodeprefix_len);
  if (n->startcodeprefix_len < 4)
  {
    offset = 1;
    length = 3;
  }

  if ( length != (int) fwrite (startcode+offset, 1, length, p_Vid->f_annexb))
  {
    printf ("Fatal: cannot write %d bytes to bitstream file, exit (-1)\n", length);
    exit (-1);
  }

  BitsWritten = (length) << 3;

  first_byte = (unsigned char) ((n->forbidden_bit << 7) | (n->nal_reference_idc << 5) | n->nal_unit_type);

  if ( 1 != fwrite (&first_byte, 1, 1, p_Vid->f_annexb))
  {
    printf ("Fatal: cannot write %d bytes to bitstream file, exit (-1)\n", 1);
    exit (-1);
  }

  BitsWritten += 8;

// printf ("First Byte %x, nal_ref_idc %x, nal_unit_type %d\n", first_byte, n->nal_reference_idc, n->nal_unit_type);

  if (n->len != fwrite (n->buf, 1, n->len, p_Vid->f_annexb))
  {
    printf ("Fatal: cannot write %d bytes to bitstream file, exit (-1)\n", n->len);
    exit (-1);
  }
  BitsWritten += n->len * 8;

  fflush (p_Vid->f_annexb);
#if TRACE
  fprintf (p_Enc->p_trace, "\n\nAnnex B NALU w/ %s startcode, len %d, forbidden_bit %d, nal_reference_idc %d, nal_unit_type %d\n\n",
    n->startcodeprefix_len == 4?"long":"short", n->len + 1, n->forbidden_bit, n->nal_reference_idc, n->nal_unit_type);
  fflush (p_Enc->p_trace);
#endif
  return BitsWritten;
}


/*!
 ********************************************************************************************
 * \brief
 *    Opens the output file for the bytestream
 *
 * \param p_Vid
 *    VideoParameters structure
 * \param Filename
 *    The filename of the file to be opened
 *
 * \return
 *    none.  Function terminates the program in case of an error
 *
 ********************************************************************************************
*/
void OpenAnnexbFile (VideoParameters *p_Vid, char *Filename)
{
  if ((p_Vid->f_annexb = fopen (Filename, "wb")) == NULL)
  {
    printf ("Fatal: cannot open Annex B bytestream file '%s', exit (-1)\n", Filename);
    exit (-1);
  }
}


/*!
 ********************************************************************************************
 * \brief
 *    Closes the output bit stream file
 *
 * \return
 *    none.  Funtion trerminates the program in case of an error
 ********************************************************************************************
*/
void CloseAnnexbFile(VideoParameters *p_Vid) 
{
  if (fclose (p_Vid->f_annexb))
  {
    printf ("Fatal: cannot close Annex B bytestream file, exit (-1)\n");
    exit (-1);
  }
}

