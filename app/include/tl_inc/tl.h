#ifndef __INTERACTION_H
#define __INTERACTION_H

#include "comdef.h"

/***********different for everyone*******************/
#define TL_KEY	"b8ef873cd59b5a222cc381f07007d299"
/****************************************************/

#define TL_API	"www.tuling123.com/openapi/api?key=%s&info=%s"

int tl_ask(char *inStr, int isUTF8, char *outStr, int outLen);

#endif


