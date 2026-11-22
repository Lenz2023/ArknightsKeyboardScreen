file(GLOB_RECURSE CPP_SOURCES "*.cpp")
file(GLOB_RECURSE C_SOURCES "*.c")

# The boot video player is intentionally excluded; startup now goes directly
# to the lightweight UI and no MPEG data or decoder is linked into firmware.
list(FILTER CPP_SOURCES EXCLUDE REGEX ".*/mpegDecoder\\.cpp$")

# Legacy fox/spartan theme assets are no longer selectable.  Excluding them
# avoids compiling several megabytes of generated image source on every build.
# Keep wifi_1..4 and wifilock: the Wi-Fi provisioning screen still uses them.
list(FILTER C_SOURCES EXCLUDE REGEX ".*/imgs/(battery_.*|img_.*)\\.c$")
list(FILTER C_SOURCES EXCLUDE REGEX ".*/img2/.*\\.c$")
list(FILTER C_SOURCES EXCLUDE REGEX ".*/img_main[12]_bg\\.c$")
list(FILTER C_SOURCES EXCLUDE REGEX ".*/font_main1_time\\.c$")

idf_component_register(
    SRCS ${CPP_SOURCES} ${C_SOURCES}
    INCLUDE_DIRS "include"
)
