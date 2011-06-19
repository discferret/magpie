--[[
#######################
# DiscFerret Disc Drive Specification File
#
# PC 3.5in Drives A and B, twisted cable or CBL-01A cable kit
#######################
]]

-- DriveSpec version flags
drivespec_version = 1.0

-- Drive types recognised by this drivespec
drivespecs = {
	vfo_8inch_0 = {
		-- Shown to user
		friendlyname		= "8-inch (Japanese \"VFO\" pinout), drive 0",
		-- Default step rate in milliseconds
		steprate			= 6.0,
		-- Spin-up time in milliseconds
		spinup				= 1000,
		-- Number of physical tracks
		tracks				= 81,
		-- Number of tracks per inch
		tpi					= 160,		-- TODO: FIXME! This is wrong.
		-- Number of physical heads
		heads				= 2,
	},

	vfo_8inch_1 = {
		-- Shown to user
		friendlyname		= "8-inch (Japanese \"VFO\" pinout), drive 1",
		-- Default step rate in milliseconds
		steprate			= 6.0,
		-- Spin-up time in milliseconds
		spinup				= 1000,
		-- Number of physical tracks
		tracks				= 81,
		-- Number of tracks per inch
		tpi					= 160,		-- TODO: FIXME! This is wrong.
		-- Number of physical heads
		heads				= 2,
	},

	vfo_8inch_2 = {
		-- Shown to user
		friendlyname		= "8-inch (Japanese \"VFO\" pinout), drive 2",
		-- Default step rate in milliseconds
		steprate			= 6.0,
		-- Spin-up time in milliseconds
		spinup				= 1000,
		-- Number of physical tracks
		tracks				= 81,
		-- Number of tracks per inch
		tpi					= 160,		-- TODO: FIXME! This is wrong.
		-- Number of physical heads
		heads				= 2,
	},

	vfo_8inch_3 = {
		-- Shown to user
		friendlyname		= "8-inch (Japanese \"VFO\" pinout), drive 3",
		-- Default step rate in milliseconds
		steprate			= 6.0,
		-- Spin-up time in milliseconds
		spinup				= 1000,
		-- Number of physical tracks
		tracks				= 81,
		-- Number of tracks per inch
		tpi					= 160,		-- TODO: FIXME! This is wrong.
		-- Number of physical heads
		heads				= 2,
	},
}

--[[
Given the drive type, track, head and sector, return a list of output pins which need to be set.
Called once per sector on hard-sectored media, once per track on soft-sectored media
--]]
function getDriveOutputs(drivetype, track, head, sector)
	pins = 0

	-- Set drive selects. These are binary-coded on VFO drives
	if drivetype == "vfo_8inch_0" then
		pins = pins + PIN_MOTEN
	elseif drivetype == "vfo_8inch_1" then
		pins = pins + PIN_MOTEN + PIN_DS0
	elseif drivetype == "vfo_8inch_2" then
		pins = pins + PIN_MOTEN + PIN_DS1
	elseif drivetype == "vfo_8inch_3" then
		pins = pins + PIN_MOTEN + PIN_DS0 + PIN_DS1
	else
		error("Unrecognised drive type '" .. drivetype .. "'.")
	end

	-- Handle side selection
	if head == 0 then
		-- do nothing, Head 0 is PIN_SIDESEL (p32) inactive/floating high
	elseif head == 1 then
		pins = pins + PIN_SIDESEL
	else
		error("Head number " .. head .. " out of range.")
	end

	-- TODO: Handle REDWC (Reduced Write Current) on the DENSITY pin

	-- That's pretty much it, unless we need to provide TG46 on the DENSITY pin.
	return pins
end

--[[
Given the current drive status flags, identify whether the drive is ready for use.
]]
function isDriveReady(drivetype, status)
	-- Hmm. For some reason the READY output on the VFO drives doesn't work.
	-- Probably need a 4-in Schmitt O/C AND gate on the adapter board.
	return true
end

