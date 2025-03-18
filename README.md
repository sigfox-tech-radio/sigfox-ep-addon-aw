# Sigfox End-Point Atlas WiFi addon (EP_ADDON_AW)

## Description

The **Sigfox End-Point Atlas WiFi addon** provides utility functions to build the specific uplink payload to use **Sigfox Atlas WiFi geolocation** service. The addon reads a list of access points scanned by a WiFi receiver, selects the best MAC addresses based on mandatory and optional filters, and builds the corresponding payload which has to be sent by the application through the [Sigfox end point library](https://github.com/sigfox-tech-radio/sigfox-ep-lib).

The access point descriptor (`SIGFOX_EP_ADDON_AW_API_access_point_t` structure) contains a `status` field which has to be initialized with the `SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_NEW` value and which will be updated by the addon after the MAC addresses selection. This way, the addon can be **called multiple times with a same access points list**.

The addon does not call the Sigfox EP library directly, for the following technical reasons:

* The addon **does not interfer with the library state machine** chosen by the device maker: an Atlas WiFi message can be sent at any time, following or preceding other custom messages. There is no need to close the library, open the addon, close the addon, re-open the library and so on.
* This **avoid the duplication of all the library API**. Indeed, if the addon calls the library directly, it should expose all the other library features (at least, the open, close and process functions, the message parameters structure, etc.). This would require multiple copy operations, which would significantly and uselessly increase the memory footprint.
* The addon **does not have any dependency on the library API functions**: it only needs some definitions taken from the `sigfox_types.h` file and few compilation flags from the `sigfox_ep_flags.h` file.

The table below shows the versions **compatibility** between this addon and the Sigfox End-Point library.

| **EP_ADDON_AW** | **EP_LIB** |
|:---:|:---:|
| [v2.0](https://github.com/sigfox-tech-radio/sigfox-ep-addon-aw/releases/tag/v2.0) | >= [v4.0](https://github.com/sigfox-tech-radio/sigfox-ep-lib/releases/tag/v4.0) |

## Stack architecture

<p align="center">
<img src="https://github.com/sigfox-tech-radio/sigfox-ep-addon-aw/wiki/images/sigfox_ep_addon_aw_architecture.drawio.png" width="600"/>
</p>

## Compilation flags for optimization

This addon inherits all the [Sigfox End-Point library flags](https://github.com/sigfox-tech-radio/sigfox-ep-lib/wiki/compilation-flags-for-optimization) and can be optimized accordingly.

The **12 bytes payload** must be available to use this addon.

## MAC address format

In this section, `Bxby` denotes the bit `y` of byte `x` of the MAC address.

A MAC address is a **6 bytes identifier** (`B0` to `B5`). The two least significant bits of the first byte are used to distinguish **unicast**, **multicast**, **universal** and **locally administered** addresses.

| `B0b7` | `B0b6` | `B0b5` | `B0b4` | `B0b3` | `B0b2` | `B0b1` | `B0b0` |
|---|---|---|---|---|---|---|---|
| x | x | x | x | x | x | `U/L` | `I/G` |

| `I/G` | MAC address type |
|---|---|
| 0 | Unicast |
| 1 | Multicast |

| `U/L` | MAC address type |
|---|---|
| 0 | Universal |
| 1 | Locally administered |

## Sigfox Atlas WiFi payload format

In this section, `Bxby` denotes the bit `y` of byte `x` of the uplink payload.

The Atlas WiFi payload always has a **fixed length of 12 bytes** (`B0` to `B11`), so that it contains **one or two MAC addresses** (in case of one, bytes `B6` to `B11` are padded with zeroes).

**Only unicast addresses must be kept** to use Sigfox WiFi geolocation service: this mandatory filter is implemented in the addon and always active. It is also **recommended to enable the other filters** provided in the addon API, but they can be adapted or enriched depending on the device application or the targeted environment.

Since multicast addresses are filtered out, the Sigfox cloud uses the `I/G` and `U/L` bits of the first MAC address as a **payload type header**, to determine if the payload contains MAC addresses to be processed by the Sigfox geomodule or if it contains custom data. Format is given in the following table:

| `B0b1` (`U/L`) | `B0b0` (`I/G`) | Payload type |
|---|---|---|
| x | 0 | **Atlas WiFi data** parsed by Sigfox geomodule |
| 0 | 1 | Reserved |
| 1 | 1 | **Custom data** not processed by Sigfox geomodule |

> [!NOTE]
> Custom messages which does not contain Atlas WiFi data are sent by **calling the** [Sigfox End-Point library](https://github.com/sigfox-tech-radio/sigfox-ep-lib) **directly**. In case of a **12 bytes payload, ensure that both** `B0b1` **and** `B0b0` **bits are set**.

## Configuring the filters

Optional MAC addresses filtering is configured by calling the `SIGFOX_EP_ADDON_AW_API_set_filter()` function which takes two arguments:

* `filters` which is read as a **bitfield**: bit `i` enables or disables the filter `i` of the `SIGFOX_EP_ADDON_AW_API_filter_t` enumeration (see example below). Value `0` therefore disables all the optional filters. This step is used to **remove some MAC addresses** from the input list according to several criterias.
* `sorting` which is one of the `SIGFOX_EP_ADDON_AW_API_sorting_t` enumeration values. This second step is used to **order the remaining valid addresses**. When `SIGFOX_EP_ADDON_AW_API_SORTING_NONE` is selected, MAC addresses are taken in the same order as the input list.

The following example gives the **recommended configuration** where all the filters are enabled:

```c
SIGFOX_EP_ADDON_AW_API_status_t sigfox_ep_addon_aw_status = SIGFOX_EP_ADDON_AW_API_SUCCESS;
sfx_u8 filters = 0;

filters |= (1 << SIGFOX_EP_ADDON_AW_API_FILTER_LOCALLY_ADMINISTERED);
filters |= (1 << SIGFOX_EP_ADDON_AW_API_FILTER_SSID_EMPTY);
filters |= (1 << SIGFOX_EP_ADDON_AW_API_FILTER_SSID_BLACK_LIST);
sigfox_ep_addon_aw_status = SIGFOX_EP_ADDON_AW_API_set_filter(filters, SIGFOX_EP_ADDON_AW_API_SORTING_RSSI);
```

## Building the payload

The addon takes a **pointer to a list of access point pointers**, then builds the **12 bytes payload** and returns the **effective number of MAC addresses** used in the payload.

```c
// Local variables.
SIGFOX_EP_ADDON_AW_API_status_t sigfox_ep_addon_aw_status = SIGFOX_EP_ADDON_AW_API_SUCCESS;
SIGFOX_EP_ADDON_AW_API_input_data_t input_data;
sfx_u8 ul_payload_wifi[SIGFOX_EP_ADDON_AW_API_UL_PAYLOAD_SIZE_BYTES];
sfx_u8 nb_mac_ul_payload = 0;
// Access points.
SIGFOX_EP_ADDON_AW_API_access_point_t access_point_0 = { "C4:01:23:45:67:89", "ssid_0", -74, SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_NEW };
SIGFOX_EP_ADDON_AW_API_access_point_t access_point_1 = { "C4:AB:CD:EF:01:23", "ssid_1", -55, SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_NEW };
SIGFOX_EP_ADDON_AW_API_access_point_t access_point_2 = { "C4:45:67:89:AB:CD", "ssid_2", -69, SIGFOX_EP_ADDON_AW_API_ACCESS_POINT_STATUS_NEW };
// Build list.
SIGFOX_EP_ADDON_AW_API_access_point_t* access_point_list[] = { &access_point_0, &access_point_1, &access_point_2 };
// Build input data structure.
input_data.access_point_list = (SIGFOX_EP_ADDON_AW_API_access_point_t**) access_point_list;
input_data.access_point_list_size = 3;
// Build the payload.
sigfox_ep_addon_aw_status = SIGFOX_EP_ADDON_AW_API_build_ul_payload(&input_data, (sfx_u8*) ul_payload_wifi, &nb_mac_ul_payload);
```

## Sending the data

The payload can now be **sent to the Sigfox network using the device library**. The following code example only shows the payload and the payload size fields which are specifically taken from the addon: see the [Sigfox End-Point library documentation](https://github.com/sigfox-tech-radio/sigfox-ep-lib/wiki/basic-examples) for more details about the other parameters of the message structure.

```c
// Local variables.
SIGFOX_EP_API_status_t sigfox_ep_api_status;
SIGFOX_EP_API_application_message_t application_message;
// Build message.
...
application_message.ul_payload = ul_payload_wifi;
application_message.ul_payload_size_bytes = SIGFOX_EP_ADDON_AW_API_UL_PAYLOAD_SIZE_BYTES;
...
// Send message
sigfox_ep_api_status = SIGFOX_EP_API_send_application_message(&application_message);
```

## How to add Sigfox Atlas WiFi addon to your project

### Dependencies

The only dependency of this addon is the `sigfox_types.h` file of the [Sigfox End-Point library](https://github.com/sigfox-tech-radio/sigfox-ep-lib).

### Submodule

The best way to embed the Sigfox End-Point Atlas WiFi addon into your project is to use a [Git submodule](https://git-scm.com/book/en/v2/Git-Tools-Submodules), in a similar way to the library. The addon will be seen as a sub-repository with independant history. It will be much easier to **upgrade the addon** or to **switch between versions** when necessary, by using the common `git pull` and `git checkout` commands within the `sigfox-ep-addon-aw` folder.

To add the Sigfox type approval addon submodule, go to your project location and run the following commands:

```bash
mkdir lib
cd lib/
git submodule add https://github.com/sigfox-tech-radio/sigfox-ep-addon-aw.git
```

This will clone the Sigfox End-Point Atlas WiFi l addon repository. At project level, you can commit the submodule creation with the following commands:

```bash
git commit --message "Add Sigfox End Point Atlas WiFi addon submodule."
git push
```

With the submodule, you can easily:

* Update the addon to the **latest version**:

```bash
cd lib/sigfox-ep-addon-aw/
git pull
git checkout master
```

* Use a **specific release**:

```bash
cd lib/sigfox-ep-addon-aw/
git pull
git checkout <tag>
```

### Raw source code

You can [download](https://github.com/sigfox-tech-radio/sigfox-ep-addon-aw/releases) or clone any release of the Sigfox End-Point Atlas WiFi addon and copy all files into your project.

```bash
git clone https://github.com/sigfox-tech-radio/sigfox-ep-addon-aw.git
```

### Precompiled source code

You can [download](https://github.com/sigfox-tech-radio/sigfox-ep-addon-aw/releases) or clone any release of the Sigfox End-Point Atlas WiFi addon and copy all files into your project. If you do not plan to change your compilation flags in the future, you can perform a **precompilation step** before copying the file in your project. The precompilation will **remove all preprocessor directives** according to your flags selection, in order to produce a more **readable code**. Then you can copy the new files into your project.

```bash
git clone https://github.com/sigfox-tech-radio/sigfox-ep-addon-aw.git
```

To perform the precompilation, you have to install `cmake` and `unifdef` tools, and run the following commands:

```bash
cd sigfox-ep-addon-aw/
mkdir build
cd build/

cmake -DSIGFOX_EP_LIB_DIR=<sigfox-ep-lib path> \
      -DSIGFOX_EP_RC1_ZONE=ON \
      -DSIGFOX_EP_RC2_ZONE=ON \
      -DSIGFOX_EP_RC3_LBT_ZONE=ON \
      -DSIGFOX_EP_RC3_LDC_ZONE=ON \
      -DSIGFOX_EP_RC4_ZONE=ON \
      -DSIGFOX_EP_RC5_ZONE=ON \
      -DSIGFOX_EP_RC6_ZONE=ON \
      -DSIGFOX_EP_RC7_ZONE=ON \
      -DSIGFOX_EP_APPLICATION_MESSAGES=ON \
      -DSIGFOX_EP_CONTROL_KEEP_ALIVE_MESSAGE=ON \
      -DSIGFOX_EP_BIDIRECTIONAL=ON \
      -DSIGFOX_EP_ASYNCHRONOUS=ON \
      -DSIGFOX_EP_LOW_LEVEL_OPEN_CLOSE=ON \
      -DSIGFOX_EP_REGULATORY=ON \
      -DSIGFOX_EP_LATENCY_COMPENSATION=ON \
      -DSIGFOX_EP_SINGLE_FRAME=ON \
      -DSIGFOX_EP_UL_BIT_RATE_BPS=OFF \
      -DSIGFOX_EP_TX_POWER_DBM_EIRP=OFF \
      -DSIGFOX_EP_T_IFU_MS=OFF \
      -DSIGFOX_EP_T_CONF_MS=OFF \
      -DSIGFOX_EP_UL_PAYLOAD_SIZE=OFF \
      -DSIGFOX_EP_AES_HW=ON \
      -DSIGFOX_EP_CRC_HW=OFF \
      -DSIGFOX_EP_MESSAGE_COUNTER_ROLLOVER=OFF \
      -DSIGFOX_EP_PARAMETERS_CHECK=ON \
      -DSIGFOX_EP_CERTIFICATION=ON \
      -DSIGFOX_EP_PUBLIC_KEY_CAPABLE=ON \
      -DSIGFOX_EP_VERBOSE=ON \
      -DSIGFOX_EP_ERROR_CODES=ON \
      -DSIGFOX_EP_ERROR_STACK=12 ..

make precompil_sigfox_ep_addon_aw
```

The new files will be generated in the `build/precompil` folder.

### Static library

You can also [download](https://github.com/sigfox-tech-radio/sigfox-ep-addon-aw/releases) or clone any release of the Sigfox End-Point Atlas WiFi addon and build a **static library**.

```bash
git clone https://github.com/sigfox-tech-radio/sigfox-ep-addon-aw.git
```

To build a static library, you have to install `cmake` tool and run the following commands:

```bash
cd sigfox-ep-addon-aw/
mkdir build
cd build/

cmake -DSIGFOX_EP_LIB_DIR=<sigfox-ep-lib path> \
      -DSIGFOX_EP_RC1_ZONE=ON \
      -DSIGFOX_EP_RC2_ZONE=ON \
      -DSIGFOX_EP_RC3_LBT_ZONE=ON \
      -DSIGFOX_EP_RC3_LDC_ZONE=ON \
      -DSIGFOX_EP_RC4_ZONE=ON \
      -DSIGFOX_EP_RC5_ZONE=ON \
      -DSIGFOX_EP_RC6_ZONE=ON \
      -DSIGFOX_EP_RC7_ZONE=ON \
      -DSIGFOX_EP_APPLICATION_MESSAGES=ON \
      -DSIGFOX_EP_CONTROL_KEEP_ALIVE_MESSAGE=ON \
      -DSIGFOX_EP_BIDIRECTIONAL=ON \
      -DSIGFOX_EP_ASYNCHRONOUS=ON \
      -DSIGFOX_EP_LOW_LEVEL_OPEN_CLOSE=ON \
      -DSIGFOX_EP_REGULATORY=ON \
      -DSIGFOX_EP_LATENCY_COMPENSATION=ON \
      -DSIGFOX_EP_SINGLE_FRAME=ON \
      -DSIGFOX_EP_UL_BIT_RATE_BPS=OFF \
      -DSIGFOX_EP_TX_POWER_DBM_EIRP=OFF \
      -DSIGFOX_EP_T_IFU_MS=OFF \
      -DSIGFOX_EP_T_CONF_MS=OFF \
      -DSIGFOX_EP_UL_PAYLOAD_SIZE=OFF \
      -DSIGFOX_EP_AES_HW=ON \
      -DSIGFOX_EP_CRC_HW=OFF \
      -DSIGFOX_EP_MESSAGE_COUNTER_ROLLOVER=OFF \
      -DSIGFOX_EP_PARAMETERS_CHECK=ON \
      -DSIGFOX_EP_CERTIFICATION=ON \
      -DSIGFOX_EP_PUBLIC_KEY_CAPABLE=ON \
      -DSIGFOX_EP_VERBOSE=ON \
      -DSIGFOX_EP_ERROR_CODES=ON \
      -DSIGFOX_EP_ERROR_STACK=12 ..

make sigfox_ep_addon_aw
```

The archive will be generated in the `build/lib` folder.
