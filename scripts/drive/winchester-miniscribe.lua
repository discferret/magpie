--[[
#######################
# DiscFerret Disc Drive Specification File
#
# Winchester (ST-506/ST-412) type disc drives with ST-506 adapter
#######################
]]

-- DriveSpec version flags
drivespec_version = 1.0

-- Drive types recognised by this drivespec
drivespecs = {
	-- Drive type ID maps to a table
	miniscribe_3085 = {
		-- Shown to user
		friendlyname		= "WINCHESTER: Miniscribe 3085",
		-- Default step rate in milliseconds (250us)
		steprate			= 0.250,
		-- Spin-up time in milliseconds -- zero because the drive signals READY
		spinup				= 0,
		-- Number of tracks
		tracks				= 1170,
		-- Number of tracks per inch. Zero means "do not enforce" (but there will be a warning in the log file)
		tpi					= 0,
		-- Number of heads
		heads				= 7
	},
}

--[[
Given the drive type, track, head and sector, return a list of output pins which need to be set.
Called once per sector on hard-sectored media, once per track on soft-sectored media
--]]
function getDriveOutputs(drivetype, track, head, sector)
	pins = 0

	if drivetype == "miniscribe_3085" then
		-- Miniscribe 3085
		pins = pins + PIN_DS0
	else
		error("Unrecognised drive type '" .. drivetype .. "'.")
	end

	-- Handle side selection
	if head > drivespecs[drivetype]['heads'] then error("Head number " .. head .. " out of range.") end
	if (bit.band(head, 1) ~= 0) then pins = pins + PIN_SIDESEL end
	if (bit.band(head, 2) ~= 0) then pins = pins + PIN_DS1 end
	if (bit.band(head, 4) ~= 0) then pins = pins + PIN_DS2 end
	if (bit.band(head, 8) ~= 0) then pins = pins + PIN_DS3 end

	return pins
end

--[[
Given the current drive status flags, identify whether the drive is ready for use.
]]
function isDriveReady(drivetype, status)
	-- DENSITY ==> Seek Complete
	-- RY/DCHG ==> Drive Ready
	if bit.band(status, STATUS_DENSITY + STATUS_READY_DCHG) == (STATUS_DENSITY + STATUS_READY_DCHG) then
		return true
	else
		return false
	end
end

