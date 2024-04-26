#ifndef _UATTR_H_
#define _UATTR_H_

#define ATTR_UMAIN_TEXT __attribute__((section(".umaintext")))
#define ATTR_UMAIN_DATA __attribute__((section(".umaindata")))

#define ATTR_ULIB_TEXT __attribute__((section(".ulibtext")))
#define ATTR_ULIB_DATA __attribute__((section(".ulibdata")))

#define ATTR_UFREEZONE_TEXT __attribute__((section(".ufreezonetext")))
#define ATTR_UFREEZONE_DATA __attribute__((section(".ufreezonedata")))

#define ATTR_ULIB1_TEXT __attribute__((section(".ulib1text")))
#define ATTR_ULIB1_DATA __attribute__((section(".ulib1data")))

#define ATTR_ULIB2_TEXT __attribute__((section(".ulib2text")))
#define ATTR_ULIB2_DATA __attribute__((section(".ulib2data")))

#endif