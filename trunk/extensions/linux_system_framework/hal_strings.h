/*
  Copyright 2007 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/**
 * Constant strings used to represent Hal properties.
 */

#ifndef EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_HAL_STRINGS_H__
#define EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_HAL_STRINGS_H__

namespace ggadget {
namespace framework {
namespace linux_system {

const char kHalDBusName[]               = "org.freedesktop.Hal";

const char kHalObjectComputer[]         = "/org/freedesktop/Hal/devices/computer";
const char kHalObjectManager[]          = "/org/freedesktop/Hal/Manager";

const char kHalInterfaceDevice[]        = "org.freedesktop.Hal.Device";
const char kHalInterfaceManager[]       = "org.freedesktop.Hal.Manager";

const char kHalMethodGetProperty[]      = "GetProperty";
const char kHalMethodGetPropertyInt[]   = "GetPropertyInteger";
const char kHalMethodFindDeviceByCapability[] = "FindDeviceByCapability";
const char kHalMethodQueryCapability[]  = "QueryCapability";

const char kHalPropInfoCapabilities[]   = "info.capabilities";
const char kHalPropInfoCategory[]       = "info.category";

const char kHalPropSystemUUID[]         = "system.hardware.uuid";
const char kHalPropSystemUUIDOld[]      = "smbios.system.uuid";
const char kHalPropSystemVendor[]       = "system.hardware.vendor";
const char kHalPropSystemVendorOld[]    = "system.vendor";
const char kHalPropSystemProduct[]      = "system.hardware.product";
const char kHalPropSystemProductOld[]   = "system.product";

const char kHalPropNet80203[]           = "net.80203";
const char kHalPropNet80203Link[]       = "net.80203.link";
const char kHalPropNet80211[]           = "net.80211";
const char kHalPropNetBlueTooth[]       = "net.bluetooth";
const char kHalPropNetIrda[]            = "net.irda";
const char kHalPropNetInterfaceUp[]     = "net.interface_up";
const char kHalPropNetInterface[]       = "net.interface";
const char kHalPropNetMedia[]           = "net.media";
const char kHalPropNetOriginalDevice[]  = "net.originating_device";

const char kHalPropBatteryType[]                 = "battery.type";
const char kHalPropBatteryIsRechargeable[]       = "battery.is_rechargeable";
const char kHalPropBatteryRechargableIsCharging[] =
    "battery.rechargeable.is_charging";
const char kHalPropBatteryRechargableIsDischarging[] =
    "battery.rechargeable.is_discharging";
const char kHalPropBatteryChargeLevelUnit[]     = "battery.charge_level.unit";
const char kHalPropBatteryChargeLevelRate[]     = "battery.charge_level.rate";
const char kHalPropBatteryChargeLevelDesign[]   = "battery.charge_level.design";
const char kHalPropBatteryChargeLevelCurrent[]  = "battery.charge_level.current";
const char kHalPropBatteryChargeLevelLastFull[] = "battery.charge_level.last_full";
const char kHalPropBatteryChargeLevelPercentage[] =
    "battery.charge_level.percentage";
const char kHalPropBatteryRemainingTime[]       = "battery.remaining_time";
const char kHalPropACAdapterPresent[]           = "ac_adapter.present";

const char kHalCapabilityACAdapter[]    = "ac_adapter";
const char kHalCapabilityBattery[]      = "battery";
const char kHalCapabilityNet[]          = "net";

const char kNetworkManagerDBusName[]       = "org.freedesktop.NetworkManager";
const char kNetworkManagerObjectPath[]     = "/org/freedesktop/NetworkManager";
const char kNetworkManagerInterface[]      = "org.freedesktop.NetworkManager";
const char kNetworkManagerDeviceInterface[]  = "org.freedesktop.NetworkManager.Devices";

const char kNetworkManagerInfoDBusName[]   = "org.freedesktop.NetworkManagerInfo";
const char kNetworkManagerInfoObjectPath[] = "/org/freedesktop/NetworkManagerInfo";
const char kNetworkManagerInfoInterface[]  = "org.freedesktop.NetworkManagerInfo";

const char kNetworkManagerMethodGetDevices[]    = "getDevices";
const char kNetworkManagerMethodGetProperties[] = "getProperties";
const char kNetworkManagerMethodSetActive[]     = "setActiveDevice";
const char kNetworkManagerMethodSetWireless[]   = "setWirelessEnabled";
const char kNetworkManagerMethodGetWireless[]   = "getWirelessEnabled";

} // namespace linux_system
} // namespace framework
} // namespace ggadget

#endif // EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_HAL_STRINGS_H__
