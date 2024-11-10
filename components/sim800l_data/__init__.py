from esphome import automation
import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TRIGGER_ID, CONF_URL

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@christianhubmann"]

CONF_APN = "apn"
CONF_APN_USER = "apn_user"
CONF_APN_PASSWORD = "apn_password"
CONF_ON_HTTP_GET_DONE = "on_http_get_done"
CONF_ON_HTTP_GET_FAILED = "on_http_get_failed"

sim800l_data_ns = cg.esphome_ns.namespace("sim800l_data")
Sim800LDataComponent = sim800l_data_ns.class_("Sim800LDataComponent", cg.Component)

# Send a HTTP GET request over GPRS.
HttpGetAction = sim800l_data_ns.class_("HttpGetAction", automation.Action)

# This automation triggers when the HTTP GET request was sent successfully.
# This does not mean that the remote server returned a success status code.
HttpGetDoneTrigger = sim800l_data_ns.class_(
    "HttpGetDoneTrigger",
    automation.Trigger.template(cg.uint16),
)

# This automation triggers when the HTTP GET request could not be sent.
HttpGetFailedTrigger = sim800l_data_ns.class_(
    "HttpGetFailedTrigger",
    automation.Trigger.template(),
)


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Sim800LDataComponent),
            cv.Optional(CONF_APN): cv.All(cv.string, cv.Length(max=64)),
            cv.Optional(CONF_APN_USER): cv.All(cv.string, cv.Length(max=32)),
            cv.Optional(CONF_APN_PASSWORD): cv.All(cv.string, cv.Length(max=32)),
            cv.Optional(CONF_ON_HTTP_GET_DONE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(HttpGetDoneTrigger),
                }
            ),
            cv.Optional(CONF_ON_HTTP_GET_FAILED): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(HttpGetFailedTrigger),
                }
            ),
        }
    )
    .extend(cv.polling_component_schema("10s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "sim800l_data",
    require_tx=True,
    require_rx=True,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_APN in config:
        cg.add(var.set_apn(config[CONF_APN]))
    if CONF_APN_USER in config:
        cg.add(var.set_apn_user(config[CONF_APN_USER]))
    if CONF_APN_PASSWORD in config:
        cg.add(var.set_apn_password(config[CONF_APN_PASSWORD]))
    for conf in config.get(CONF_ON_HTTP_GET_DONE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.uint16, "status_code")], conf)
    for conf in config.get(CONF_ON_HTTP_GET_FAILED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)


HTTP_GET_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(Sim800LDataComponent),
        cv.Required(CONF_URL): cv.templatable(cv.string_strict),
    }
)


@automation.register_action("sim800l_data.http_get", HttpGetAction, HTTP_GET_SCHEMA)
async def http_get_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_URL], args, cg.std_string)
    cg.add(var.set_url(template_))
    return var
