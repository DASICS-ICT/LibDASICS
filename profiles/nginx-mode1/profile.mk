# Select the legacy policy in the shared sources; no source/header overlays.
PROFILE_CONFIG := profiles/nginx-mode1/profile.mk
BUILD_DIR ?= build/nginx-mode1
CFLAGS := -O3 -g -MMD -MP -Iinclude -DDASICS_LINUX -DDASICS_NGINX_MODE1 -march=rv64gc_zba_zbb_zbc_zbs -mabi=lp64d
SOURCES += $(MODE1_SOURCES)
