#ifndef _DEBUG_H_
#define _DEBUG_H_

//#define DEBUG

#ifdef DEBUG
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
void debug_print(const char *fmt, ...);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#else /* !DEBUG */
#define debug_print(...)
#endif /* DEBUG */

#endif /* !_DEBUG_H_ */
