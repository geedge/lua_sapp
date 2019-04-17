function main()
    while true do 
        headers = get_http_request_headers(ctx)
        for k, v in pairs(headers) do
            print(k, v)
        end
    end
end


function main()
    headers = get_http_request_headers()
    content = get_http_content()
    ---handle 