/*
 * debug.h
 *
 *  Created on: Dec 4, 2014
 *      Author: Minh
 */

#define DEBUG 0

#ifndef USER_DEBUG_H_
#define USER_DEBUG_H_

#ifndef INFO
//#define INFO os_printf
#define INFO if (DEBUG == 1) os_printf
#endif

#endif /* USER_DEBUG_H_ */
