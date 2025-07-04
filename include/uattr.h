#ifndef _UATTR_H_
#define _UATTR_H_

#define ATTR_UMAIN_TEXT __attribute__((section(".umaintext")))
#define ATTR_UMAIN_DATA __attribute__((section(".umaindata")))

#define ATTR_ULIB_TEXT __attribute__((section(".ulibtext")))
#define ATTR_ULIB_DATA __attribute__((section(".ulibdata")))

#define ATTR_UFREEZONE_TEXT __attribute__((section(".ufreezonetext")))
#define ATTR_UFREEZONE_DATA __attribute__((section(".ufreezonedata")))

#define ATTR_ULIB_CALLER_TEXT __attribute__((section(".ulibtext.caller"),aligned(8)))
#define ATTR_ULIB_CALLER_DATA __attribute__((section(".ulibdata.caller"),aligned(8)))
#define ATTR_ULIB_CALLER_PAGE __attribute__((section(".ulibdata.caller"),aligned(PAGE_SIZE)))

#define ATTR_ULIB_CALLEE_TEXT __attribute__((section(".ulibtext.callee"),aligned(8)))
#define ATTR_ULIB_CALLEE_DATA __attribute__((section(".ulibdata.callee"),aligned(8)))

#define ATTR_ULIB_STATISTICS_DATA __attribute__((section(".ulibdata.statistics"),aligned(8)))

#endif