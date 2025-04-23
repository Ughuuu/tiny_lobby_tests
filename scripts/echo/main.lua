local main = {}
local system = require("system")

function main.echo(message)
    print(system.get_time_since_epoch())
    return message
end

return main
