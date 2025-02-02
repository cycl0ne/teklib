
--
--	tek.ui example
--

require "tek.ui"

-------------------------------------------------------------------------------

tek.ui.slider = tek.ui.area:new()

function tek.ui.slider:new(o)
	o = o or { }
	o.pos = { 0, 0, 0.1, 0.1 }
 	o.knob = tek.ui.button:new { 
 		margin = { 1, 1, 1, 1 }, padding = { 0, 0, 0, 0 } 
 	}
 	o.knobminw = 8
 	o.knobminh = 8
	return tek.ui.atom.new(self, o)
end

function tek.ui.slider:init(group, app)
 	self.knob:init(group, app)
	tek.ui.area.init(self, group, app)
end

function tek.ui.slider:getborderstyle()
	return tek.ui.area.getborderstyle(self) or tek.ui.borders.nice
end

function tek.ui.slider:getminmax(app)
	local tw = self.minw or self.knobminw
	local th = self.minh or self.knobminh
	local m1, m2, m3, m4 = self.knob:getmarginandborder(app)
	tw = tw + m1 + m3
	th = th + m2 + m4
	return tw, th, self.hmax and -1 or tw, self.vmax and -1 or th
end

function tek.ui.slider:draw(app, drawall)
	
	local x0 = self.rect[1]	+ self.knob.mab[1]
	local y0 = self.rect[2] + self.knob.mab[2]
	local x1 = self.rect[3] - self.knob.mab[3]
	local y1 = self.rect[4] - self.knob.mab[4]
	
	local wh = {
		self.rect[3] - self.rect[1] - self.knob.mab[3] - self.knob.mab[1]
			- self.knobminw + 1,
		self.rect[4] - self.rect[2] - self.knob.mab[4] - self.knob.mab[2]
			- self.knobminh + 1
	}
	
	if self.hfree then
		x1 = x0 + self.pos[3] * wh[1] + self.knobminw - 1
		x0 = x0 + self.pos[1] * wh[1]
	end
	
	if self.vfree then
		y1 = y0 + self.pos[4] * wh[2] + self.knobminh - 1
		y0 = y0 + self.pos[2] * wh[2]
	end

	self.knob.rect[1] = x0
	self.knob.rect[2] = y0
	self.knob.rect[3] = x1
	self.knob.rect[4] = y1
	
	local p = app.pens[self.active + 1]
	local bg = tek.ui.region:new(self.rect[1], self.rect[2], self.rect[3],
		self.rect[4])
	local b1, b2, b3, b4 = self.knob:getborder(app)
	bg:xorrect(x0 - b1, y0 - b2, x1 + b3, y1 + b4)
	
	for _, r in ipairs(bg:get()) do
		app.v:frect(r[1], r[2], r[3], r[4], p[1])
	end

	self:getborderstyle():draw(self, app)
	self.knob:draw(app, drawall)	
end

function tek.ui.slider:isclick(app, xy)
	return tek.ui.area.isover(self, app, xy)
end

function tek.ui.slider:clickstepx()
	return tek.ui.max(self.pos[3] - 
		self.pos[1], self.knobminw / (self.rect[3] - self.rect[1] + 1))
end
function tek.ui.slider:clickstepy()
	return tek.ui.max(self.pos[4] - 
		self.pos[2], self.knobminh / (self.rect[4] - self.rect[2] + 1))
end

function tek.ui.slider:clickcontainer(app, xy)
	local b1, b2, b3, b4 = self.knob:getborder(app)
	local newposx, newposy 
	if self.hfree then
		if xy[1] < self.knob.rect[1] - b1 then
			newposx = self.pos[1] - self:clickstepx()
		elseif xy[1] > self.knob.rect[3] + b3 then
			newposx = self.pos[1] + self:clickstepx()
		end		
	end
	if self.vfree then
		if xy[2] < self.knob.rect[2] - b2 then
			newposy = self.pos[2] - self:clickstepy()
		elseif xy[2] > self.knob.rect[4] + b4 then
			newposy = self.pos[2] + self:clickstepy()
		end		
	end
	if newposx or newposy then
		return self:setpos(newposx, newposy)
	end
end

function tek.ui.slider:onhold(app, xy, ticks)
	if not self.move0 then
		return self:clickcontainer(app, self.holdxy)
	end
end

function tek.ui.slider:onpress(app, xy)
	self.holdxy = { xy[1], xy[2] }
	return self:clickcontainer(app, xy)
end

function tek.ui.slider:startmove(app, xy)
	local b1, b2, b3, b4 = self.knob:getborder(app)
	if xy[1] >= self.knob.rect[1] - b1 and xy[1] <= self.knob.rect[3] + b3 and
		xy[2] >= self.knob.rect[2] - b2 and xy[2] <= self.knob.rect[4] + b4 then
	 	self.move0 = { xy[1], xy[2] }
	 	self.rect0 = { self.pos[1], self.pos[2], self.pos[3], self.pos[4] }
		return true
	end
end

function tek.ui.slider:domove(app, xy)
	local newx, newy
	if self.hfree then
		local w = self.rect[3] - self.rect[1] - self.knob.mab[3]
			- self.knob.mab[1]	- self.knobminw + 1
		newx = self.rect0[1] + (xy[1] - self.move0[1]) / tek.ui.max(w, 1)
	end
	if self.vfree then
		local h = self.rect[4] - self.rect[2] - self.knob.mab[4]
			- self.knob.mab[2] - self.knobminh + 1
		newy = self.rect0[2] + (xy[2] - self.move0[2]) / tek.ui.max(h, 1)
	end
	return self:setpos(newx, newy)
end

function tek.ui.slider:endmove(app, xy)
	self.move0 = nil
end

function tek.ui.slider:setsize(xsize, ysize)
	local oldx, oldy = self.pos[3], self.pos[4]
	self.pos[3] = self.pos[1] + xsize
	if self.pos[3] > 1 then
		self.pos[3] = 1
	end
	self.pos[4] = self.pos[2] + ysize
	if self.pos[4] > 1 then
		self.pos[4] = 1
	end
	if self.pos[3] ~= oldx or self.pos[4] ~= oldy then
		self:onupdate()
		return true
	end
end

function tek.ui.slider:setpos(xpos, ypos)
	local oldx, oldy = self.pos[1], self.pos[2]
	if xpos then
		local w = self.pos[3] - self.pos[1]
 		if xpos + w > 1 then
 			xpos = 1 - w
 		elseif xpos < 0 then
 			xpos = 0
 		end
		self.pos[1] = xpos
		self.pos[3] = xpos + w
	end
	if ypos then
		local h = self.pos[4] - self.pos[2]
		if ypos + h > 1 then
			ypos = 1 - h
		elseif ypos < 0 then
			ypos = 0
		end
		self.pos[2] = ypos
		self.pos[4] = ypos + h
	end
	if self.pos[1] ~= oldx or self.pos[2] ~= oldy then
		self:onupdate()
		return true
	end
end

function tek.ui.slider:onupdate(app)
 	self.redraw = true
 	return true
end

-------------------------------------------------------------------------------

tek.ui.textfield = tek.ui.area:new()

function tek.ui.textfield:getborderstyle()
	return tek.ui.area.getborderstyle(self) or tek.ui.borders.bevel
end

function tek.ui.textfield:init(group, app)
	self.trect = { }
	self.blink = 0
	self.blinkdelay = 20
	self.blinkstate = 0
	tek.ui.area.init(self, group, app)
end

function tek.ui.textfield:getminmax(app)
 	local tw = app.fw
	local th = app.fh
	local p1, p2, p3, p4 = self:getpadding()
	tw = tw + p1 + p3
	th = th + p2 + p4
	tw = tek.ui.max(self.minw or 0, tw) 
	th = tek.ui.max(self.minh or 0, th)
	return tw, th, self.hmax and -1 or tw, self.vmax and -1 or th
end

function tek.ui.textfield:draw(app, drawall)
	local pens = app.pens[self.active + 1]
	local v = app.v
	tek.ui.area.draw(self, app, drawall)
 	if self.label then
		local p1, p2, p3, p4 = self:getpadding()
		local w = self.rect[3] - self.rect[1] + 1 - p1 - p3
		local h = self.rect[4] - self.rect[2] + 1 - p2 - p4
		local backpen = self.hilite and pens[7] or pens[1]
		
		local tw = self.label:len() * app.fw
		self.trect[1] = self.rect[1] + p1 + (w - tw) / 2
		self.trect[2] = self.rect[2] + p2 + (h - app.fh) / 2
		self.trect[3] = self.trect[1] + tw - 1
		self.trect[4] = self.trect[2] + app.fh - 1
		
		if DELAY then
			v:text(self.trect[1], self.trect[2], self.label, pens[9], pens[9])
			_tek_vis.delay(DELAY)
		end
		v:text(self.trect[1], self.trect[2], self.label, pens[2], backpen)
		
		if self.tcursor and self == app.focuselement then
			local s = self.label:sub(self.tcursor, self.tcursor)
			if s == "" then s = " " end
			if self.active == 1 or self.blinkstate == 1 then
				v:text(self.trect[1] + (self.tcursor - 1) * app.fw, self.trect[2], 
					s, pens[3], pens[9])
			else
				v:text(self.trect[1] + (self.tcursor - 1) * app.fw, self.trect[2], 
					s, pens[2], backpen)
			end
		end
		
 	end
end

function tek.ui.textfield:isclick(app, xy)
 	return tek.ui.area.isover(self, app, xy)
end

function tek.ui.textfield:onpress(app, xy)
	if xy[1] >= self.trect[1] and xy[1] <= self.trect[3] and xy[2] >= self.trect[2]
		and xy[2] <= self.trect[4] then
		self.tcursor = math.floor((xy[1] - self.trect[1]) / app.fw) + 1
	elseif xy[1] < self.trect[1] then
		self.tcursor = 1
	elseif xy[1] > self.trect[3] then
		self.tcursor = self.label:len() + 1
	end
	self.blink = self.blinkdelay
end

function tek.ui.textfield:set(s)
	self.label = s
	self.redraw = true
	return true
end

function tek.ui.textfield:interval()
	self.blink = self.blink - 1
	if self.blink < 0 then
		self.blink = self.blinkdelay
		self.blinkstate = ((self.blinkstate == 1) and 0) or 1
		return true
	end
end


-------------------------------------------------------------------------------
--
--	sample application
--

app = tek.ui.app:new {
 	width = 300, height = 500, title = "Choose file...",
	gui = tek.ui.vgroup:new {
		margin = { 1, 1, 1, 1 },
		childs = {
			tek.ui.vgroup:new {
				vmax = true, hmax = true,
				childs = {
					tek.ui.hgroup:new {
						childs = {
							tek.ui.button:new { hmax = true, vmax = true },
							tek.ui.handle:new { vmax = true },
							tek.ui.vgroup:new {
								hmax = true,
								childs = {
									tek.ui.slider:new { 
										vmax = true, hmax = true, vfree = true,
										onupdate = function(self, app)
											self.group.group.group.childs[2]:set(tostring(self.pos[2]))
											return tek.ui.slider.onupdate(self, app)
										end,
									},
									tek.ui.button:new { label = "^", hmax = true,
										do_up = function(self, app, ticks)
											local s = self.group.childs[1]
											return s:setpos(nil, s.pos[2] - s:clickstepy())
										end,
										onpress = function(self, app, xy)
											return self:do_up(app)
										end,
										onhold = function(self, app, xy, ticks)
											return self:do_up(app)
										end,
									},
									tek.ui.button:new { label = "v", hmax = true,
										do_down = function(self, app, ticks)
											local s = self.group.childs[1]
											return s:setpos(nil, s.pos[2] + s:clickstepy())
										end,
										onpress = function(self, app)
											return self:do_down(app)
										end,
										onhold = function(self, app, xy, ticks)
											return self:do_down(app)
										end,
									},
								},
							},
						},
					},
					tek.ui.textfield:new { hmax = true, label = "entry" },
				},	
			},
			
			tek.ui.hgroup:new {
				childs = {
					tek.ui.button:new { label = "Directory", hmax = true },
					tek.ui.button:new { label = "Reload" },
				},
			},
			tek.ui.hgroup:new {
				childs = {
					tek.ui.button:new { label = "Filename", hmax = true },
					tek.ui.button:new { label = "Hidden" },
				},
			},
			tek.ui.hgroup:new {
				align = 1,
				childs = {
					tek.ui.button:new { label = "Open" },
					tek.ui.textarea:new { label = "Hallo", hmax = true },
					tek.ui.button:new { label = "Cancel", 
						onrelease = function(e, app)
							abort = true
					 	end,
					},
				},
			},
		}
	}
}

-------------------------------------------------------------------------------
--
--	main loop
--

repeat
	app:wait()
	repeat
		local msg = app:getmsg()
		if msg then
			if msg[2] == 1 or
				(msg[2] == 256 and msg[3] == 27) then
				abort = true
			else
				app:passmsg(msg)
			end
		end
	until not msg
until abort

