local main = {}

-- Example of function exported that echoes a message
function main.echo(message: string)
    print("Echo: " .. message)
    -- Return the message back to the caller
    return message
end

return main
