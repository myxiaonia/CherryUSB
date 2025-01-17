#include "usbd_core.h"
#include "usbd_audio.h"

#define USBD_VID           0xffff
#define USBD_PID           0xffff
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#ifdef CONFIG_USB_HS
#define EP_INTERVAL 0x04
#else
#define EP_INTERVAL 0x01
#endif

#define AUDIO_IN_EP 0x81

/* AUDIO Class Config */
#define AUDIO_FREQ 16000U

/* AudioFreq * DataSize (2 bytes) * NumChannels (Stereo: 1) */
/* 16bit(2 Bytes) 单声道(Mono:1) */
#define AUDIO_IN_PACKET ((uint32_t)((AUDIO_FREQ * 2 * 1) / 1000))

#define USB_AUDIO_CONFIG_DESC_SIZ (unsigned long)(9 +                                       \
                                                  AUDIO_AC_DESCRIPTOR_INIT_LEN(1) +         \
                                                  AUDIO_SIZEOF_AC_INPUT_TERMINAL_DESC +     \
                                                  AUDIO_SIZEOF_AC_FEATURE_UNIT_DESC(1, 1) + \
                                                  AUDIO_SIZEOF_AC_OUTPUT_TERMINAL_DESC +    \
                                                  AUDIO_AS_DESCRIPTOR_INIT_LEN(1))

#define AUDIO_AC_SIZ (AUDIO_SIZEOF_AC_HEADER_DESC(1) +         \
                      AUDIO_SIZEOF_AC_INPUT_TERMINAL_DESC +     \
                      AUDIO_SIZEOF_AC_FEATURE_UNIT_DESC(1, 1) + \
                      AUDIO_SIZEOF_AC_OUTPUT_TERMINAL_DESC)

const uint8_t audio_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0001, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_AUDIO_CONFIG_DESC_SIZ, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    AUDIO_AC_DESCRIPTOR_INIT(0x00, 0x02, AUDIO_AC_SIZ, 0x00, 0x01),
    AUDIO_AC_INPUT_TERMINAL_DESCRIPTOR_INIT(0x01, AUDIO_INTERM_MIC, 0x01, 0x0001),
    AUDIO_AC_FEATURE_UNIT_DESCRIPTOR_INIT(0x02, 0x01, 0x01, 0x03),
    AUDIO_AC_OUTPUT_TERMINAL_DESCRIPTOR_INIT(0x03, AUDIO_TERMINAL_STREAMING, 0x02),
    AUDIO_AS_DESCRIPTOR_INIT(0x01, 0x03, 0x01, 0x81, AUDIO_IN_PACKET, EP_INTERVAL, AUDIO_SAMPLE_FREQ_3B(AUDIO_FREQ)),
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x26,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'U', 0x00,                  /* wcChar10 */
    'A', 0x00,                  /* wcChar11 */
    'C', 0x00,                  /* wcChar12 */
    ' ', 0x00,                  /* wcChar13 */
    'D', 0x00,                  /* wcChar14 */
    'E', 0x00,                  /* wcChar15 */
    'M', 0x00,                  /* wcChar16 */
    'O', 0x00,                  /* wcChar17 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '1', 0x00,                  /* wcChar3 */
    '0', 0x00,                  /* wcChar4 */
    '3', 0x00,                  /* wcChar5 */
    '1', 0x00,                  /* wcChar6 */
    '0', 0x00,                  /* wcChar7 */
    '0', 0x00,                  /* wcChar8 */
    '0', 0x00,                  /* wcChar9 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};

volatile bool tx_flag = 0;

void usbd_audio_open(uint8_t intf)
{
    tx_flag = 1;
    USB_LOG_RAW("OPEN\r\n");
}
void usbd_audio_close(uint8_t intf)
{
    USB_LOG_RAW("CLOSE\r\n");
    tx_flag = 0;
}

static usbd_class_t audio_class;
static usbd_interface_t audio_control_intf;
static usbd_interface_t audio_stream_intf;

void usbd_audio_iso_callback(uint8_t ep)
{
}

static usbd_endpoint_t audio_in_ep = {
    .ep_cb = usbd_audio_iso_callback,
    .ep_addr = AUDIO_IN_EP
};

void audio_init()
{
    usbd_desc_register(audio_descriptor);
    usbd_audio_add_interface(&audio_class, &audio_control_intf);
    usbd_audio_add_interface(&audio_class, &audio_stream_intf);
    usbd_interface_add_endpoint(&audio_stream_intf, &audio_in_ep);
    usbd_audio_add_entity(0x02, AUDIO_CONTROL_FEATURE_UNIT);

    usbd_initialize();
}

void audio_test()
{

    while (1) {
        if (tx_flag) {
        }
    }
}
