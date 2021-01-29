--[[
#######################
# DiscFerret Disc Drive Specification File
#
# Amstrad EME23x 3-inch disc drives with custom cable
#######################
]]

-- DriveSpec version flags
drivespec_version = 1.0

-- Drive types recognised by this drivespec
drivespecs = {
	amstrad_eme23x_a = {
		-- Shown to user
		friendlyname		= "Amstrad EME-23x 3.0\", CBL-02, drive A",
		-- Default step rate in milliseconds
		steprate			= 3.0,
		-- Spin-up time in milliseconds
		spinup				= 1000,
		-- Number of physical tracks
		tracks				= 81,
		-- Number of tracks per inch
		tpi					= 160,		-- TODO: FIXME! This is wrong.
		-- Number of physical heads
		heads				= 2,
	},

	amstrad_eme23x_b = {
		-- Shown to user
		friendlyname		= "Amstrad EME-23x 3.0\", CBL-02, drive B",
		-- Default step rate in milliseconds
		steprate			= 3.0,
		-- Spin-up time in milliseconds
		spinup				= 1000,
		-- Number of physical tracks
		tracks				= 81,
		-- Number of tracks per inch
		tpi					= 160,		-- TODO: FIXME! This is a guess, based on the TPI of 3.5in floppies. = (135*3.5)/3.0
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
	-- 3.5in FDD settings are really easy to set up. First start with drive selects.
	if drivetype == "amstrad_eme23x_a" then
		-- Shugart DS0 = motor enable A
		pins = pins + PIN_MOTEN + PIN_DS0
	elseif drivetype == "amstrad_eme23x_b" then
		-- Shugart DS1 = drive select B
		pins = pins + PIN_MOTEN + PIN_DS1
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

	-- That's pretty much it, unless we need to provide TG46 on the DENSITY pin.
	return pins
end

--[[
Given the current drive status flags, identify whether the drive is ready for use.
]]
function isDriveReady(drivetype, status)
	-- Amstrad 3in drives have a READY output. We may as well use it.
	return (bit.band(status, STATUS_READY_DCHG) ~= 0)
end

