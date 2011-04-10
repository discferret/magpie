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
	pc35a = {
		-- Shown to user
		friendlyname		= "PC 3.5\", twisted cable (CBL-01A), drive A",
		-- Default step rate in milliseconds
		steprate			= 3.0,
		-- Spin-up time in milliseconds
		spinup				= 1000,
		-- Number of physical tracks
		tracks				= 84,
		-- Number of tracks per inch
		tpi					= 135,
		-- Number of physical heads
		heads				= 2,
	},

	pc35b = {
		-- Shown to user
		friendlyname		= "PC 3.5\", twisted cable (CBL-01A), drive B",
		-- Default step rate in milliseconds
		steprate			= 3.0,
		-- Spin-up time in milliseconds
		spinup				= 1000,
		-- Number of physical tracks
		tracks				= 84,
		-- Number of tracks per inch
		tpi					= 135,
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
	if drivetype == "pc35a" then
		-- Shugart DS0 = motor enable A, DS2 = drive select A
		pins = pins + PIN_DS0 + PIN_DS2
	elseif drivetype == "pc35b" then
		-- Shugart DS1 = drive select B, MOTEN = motor enable B
		pins = pins + PIN_DS1 + PIN_MOTEN
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
	-- 3.5in drives don't generally have a working READY output, and we don't give a damn about DISK CHANGE.
	-- If this were a Winchester drive, we'd be checking READY and SEEK COMPLETE.
	return true
end

