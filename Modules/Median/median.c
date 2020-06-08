/*! @file median.c
**
**  @brief Median filter.
**
**  This contains the functions for performing a median filter on byte-sized data.
**
** @author by Alexander Tran 12610655 and Hubert Chau 12568681
** @date 2020-05-22
*/
/*!
**  @addtogroup Median_Module Median module documentation
**  @{
*/
#include "Median\median.h"

/*! @brief Median filters 3 bytes.
 *
 *  @param n1 is the first  of 3 bytes for which the median is sought.
 *  @param n2 is the second of 3 bytes for which the median is sought.
 *  @param n3 is the third  of 3 bytes for which the median is sought.
 */
uint8_t Median_Filter3(const uint8_t n1, const uint8_t n2, const uint8_t n3)
{
	if (((n1 >= n2) && (n1 <= n3)) || ((n1 <= n2) && (n1 >= n3)))
	{
       return n1;
	}
	else if ((n2 >= n1 && n2 <= n3) || (n2 <= n1 && n2 >= n3))
	{
	   return n2;
	}
	else
	{
	   return n3;
	}
}

