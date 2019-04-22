
function printf(s,...)
    io.write(s:format(...))
    io.flush()
end

function format_write_file(file, s, ...)
    file:write(s:format(...))
    file:flush()
end

function init()
    ---init something
end

function print_header(header)
    for k, v in pairs(header) do
        if k == "HTTP_OTHER_REGIONS" then
            format_write_file(file, "%s:    ", k)
            for k, v in pairs(header["HTTP_OTHER_REGIONS"]) do
                format_write_file(file, "{%s}   ", v)
            end
            format_write_file(file, "\n")
        else
            format_write_file(file, "%s:    %s\n", k, v)
        end
    end
end

function process()
    file = io.open("./log/lua/http_response.log", "a+")
    format_write_file(file, "lua: call process\n")

    format_write_file(file, "\nprint stream info: \n")
    stream_info = get_stream_info()
    for k, v in pairs(stream_info) do
        format_write_file(file, "%s:  %s\n", k, v)
    end

    local resp_regions = {"HTTP_SERVER", "HTTP_RES_LINE", "HTTP_OTHER_REGIONS"}
    format_write_file(file, "\nprint response headers: \n")
    resp_header = get_http_response_header(resp_regions) 
    print_header(resp_header)

    format_write_file(file, "\nprint response body: \n")
    while true do 
        body = get_http_response_body()
        for k, v in pairs(body) do
            format_write_file(file, "%s:  %s\n", k, v)
        end
        if body['data_end'] == true then
            format_write_file(file, "response body end\n")
            break
        end
    end
    format_write_file(file, "\nlua: process end\n")
end