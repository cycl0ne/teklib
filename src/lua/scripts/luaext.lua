
---------------------------------------------------------------------
--	Extension test
---------------------------------------------------------------------

print("Lua extenstion test script " .. arg[0] .. " running")


win = tek.openwin()
if win == nil then
	print("window open failed")
else
	print("have window")
end

test1 = win:newtest(1)
test2 = win:newtest(2)

test1:print()
test2:print()


tek.closewin(win)
--win:close()

av,sum = average(1,2,3,4,5,6,7,8,9,10)
print("average()    : " .. av .. " sum: " .. sum)

av,sum = tek.average(1,2,3,4,5,6,7,8,9,10)
print("tek.average(): " .. av .. " sum: " .. sum)

print("script done.")
