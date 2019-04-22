
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
    printf("\nprint stream: \n")
    stream_info = get_stream_info()
    for k, v in pairs(stream_info) do
        printf("%s: %s\n", k, v)
    end

    printf("\nprint header:\n")
    local regions = {"ALL"}
    header = get_http_request_header(regions)
    for k, v in pairs(header) do
        printf("%s: %s\n", k, v)
    end 

    printf("\nprint body: \n")
    while true do 
        body = get_http_request_body()
        for k, v in pairs(body) do 
            printf("%s: %s\n", k, v)
        end
        if body['data_end'] == true then
            break
        end
    end

    --[[
    file = io.open("./log/lua/http_request.log", "a+")
    format_write_file(file, "lua: call process\n")

    format_write_file(file, "\nprint stream info: \n")
    stream_info = get_stream_info()
    for k, v in pairs(stream_info) do
        format_write_file(file, "%s:  %s\n", k, v)
    end

    format_write_file(file, "\nprint request headers: \n")
    local req_regions = {"HTTP_HOST", "HTTP_REQ_LINE", "HTTP_OTHER_REGIONS"}
    req_header = get_http_request_header(req_regions)
    print_header(req_header)
    

    format_write_file(file, "\nprint request body: \n")
    while true do 
        body = get_http_request_body()
        for k, v in pairs(body) do
            format_write_file(file, "%s:  %s\n", k, v)
        end
        if body['data_end'] == true then
            format_write_file(file, "request body end\n")
            break
        end
        format_write_file(file, "\n");
    end
    format_write_file(file, "\nlua: process end\n")
    io.close(file)
    ]]

end