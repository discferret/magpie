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
	{
		-- Passed to Drivespec functions
		drivetype			= "cdc-94205-51",
		-- Shown to user
		friendlyname		= "WINCHESTER: CDC / MPI / Imprimis 94205-51, Seagate ST253",
		-- Default step rate in milliseconds (250us)
		steprate			= 0.250,
		-- Spin-up time in milliseconds -- zero because the drive signals READY
		spinup				= 0,
		-- Number of tracks
		tracks				= 989,
		-- track stepping -- 1=singlestep, 2=doublestep
		trackstep			= 1,
		-- Number of heads
		heads				= 5
	},
}

--[[
Given the drive type, track, head and sector, return a list of output pins which need to be set.
Called once per sector on hard-sectored media, once per track on soft-sectored media
--]]
function getDriveOutputs(drivetype, track, head, sector)
	pins = 0

	if drivetype == "cdc-94205-51" then
		-- CDC 94205-51, Seagate ST253
		pins = pins or PIN_DS0
	else
		error("Unrecognised drive type '" .. drivetype .. "'.")
	end

	-- Handle side selection
	if head > 15 then error("Head number " .. head .. " out of range.") end
	if (head and 1) then pins = pins + PIN_SIDESEL end
	if (head and 2) then pins = pins + PIN_DS1 end
	if (head and 4) then pins = pins + PIN_DS2 end
	if (head and 8) then pins = pins + PIN_DS3 end

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

