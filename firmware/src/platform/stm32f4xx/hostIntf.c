#include <hostIntf.h>
#include <hostIntf_priv.h>
#include <variant/inc/variant.h>
#include <plat/inc/bl.h>

const struct HostIntfComm *platHostIntfInit()
{
    return hostIntfI2cInit(PLATFORM_HOST_INTF_I2C_BUS);
}

uint16_t platHwType(void)
{
    return PLATFORM_HW_TYPE;
}

uint16_t platHwVer(void)
{
    return PLATFORM_HW_VER;
}

uint16_t platBlVer()
{
    return BL.blGetVersion();
}
