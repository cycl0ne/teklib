
---------------------------------------------------------------------
--	dots test - to be used with the luathreadtest example
---------------------------------------------------------------------

NUMDOTS = 20

dots = {}
for i = 1, NUMDOTS do
	dots[i] = test.newdot(math.mod(i, 3)+1)
end

sin1, sin2, sin3, sin4 = 0, 0, 0, 0
count = 0

repeat

	s1, s2, s3, s4 = sin1, sin2, sin3, sin4

	for i = 1, NUMDOTS do

		x = (math.sin(s1) + math.sin(s2)) / 4 + 0.5
		y = (math.sin(s3) + math.sin(s4)) / 4 + 0.5
		dots[i]:setpos(x, y)
		s1 = s1 + 0.13
		s2 = s2 + 0.27
		s3 = s3 + 0.38
		s4 = s4 + 0.06
		
	end

	sin1 = sin1 + 0.074
	sin2 = sin2 + 0.031
	sin3 = sin3 + 0.056
	sin4 = sin4 + 0.042

	abort = test.delay(0.01)
	count = count + 1
	if count > 200 then abort = true end

until abort == true

print("script complete.")

--test.abort()		-- cause the main program to abort

