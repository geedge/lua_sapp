

printf = function(s,...)
            return io.write(s:format(...))
         end

function test(id)
    i = 0
    local j = 1
    while true do
        if j % 10000 == 5 and id % 10000 == 5 then
            printf("call test, coroutine id: %d, local index: %d, global index: %d\n", id, j, i)
        end
        j = j + 1
        i = i + 1
        coroutine.yield()
    end
end

function foo(x)
    if x % 10000 == 5 then
        printf("cal foo: x = %d\n", x)
    end
    coroutine.yield()
end

function foo1()
    print("call foo1")
    foo(10)
    return 3
end

function main()
    n = 10000000
    co_list = {}
    for i = n, 1, -1 do
        co = coroutine.create(function (i)
            foo(i)
        end)
        table.insert(co_list, co);
    end
    while(true) do
        for i = 1, #co_list do
            coroutine.resume(co_list[i], i)
        end
    end
end

function lua_call_c()
    
    printf("lua:  call lua_call_c\n")
    myfoo()
    printf("lua:  after my_foo\n")
end