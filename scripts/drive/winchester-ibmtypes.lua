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
-- This is set up in the function below.
drivespecs = {}

-- Iterator function
function values(t)
	local i=0
	return function() i=i+1; return t[i] end
end

-- Initialisation function
function init()
	-- PC drive types
	local ctp = {
	-- all 17 sectors per track, 512 byte sectors
	--	type	cyls	heads
		1,		306,	4,
		2,		615,	4,
		3,		615,	6,
		4,		940,	8,
		5,		940,	6,
		6,		615,	4,
		7,		462,	8,
		8,		733,	5,
		9,		900,	15,
		10,		820,	3,
		11,		855,	5,
		12,		855,	7,
		13,		306,	8,
		14,		733,	7,
	-- DT15 is reserved by the IBM BIOS. Not sure what for...
		16,		612,	4,
		17,		977,	5,
		18,		977,	7,
		19,		1024,	7,
		20,		733,	5,
		21,		733,	7,
		22,		733,	5,		-- same as 20, different Landing Zone
		23,		306,	4,
		24,		925,	7,
		25,		925,	9,
		26,		754,	7,
		27,		754,	11,
		28,		699,	7,
		29,		823,	10,
		30,		918,	7,
		31,		1024,	11,
		32,		1024,	15,
		33,		1024,	5,
		34,		612,	7,
		35,		1024,	9,
		36,		1024,	8,
		37,		615,	8,
		38,		987,	3,
		39,		987,	7,
		40,		820,	6,
		41,		977,	5,
		42,		981,	5,
		43,		830,	7,
		44,		830,	10,
		45,		917,	15,
		46,		1224,	15
	}

	-- Generate the drivespec table -- TODO: is there a better way to do this...?
	iter = values(ctp)
	while true do
		local dtype = iter()	-- drive type

		if dtype == nil then
			break
		end

		local ncyls = iter()	-- number of cylinders
		local nheads = iter()	-- number of heads

		-- calculate drive size in MBytes
		local mbytes = (ncyls * nheads * 17 * 512) / 1024 / 1024

		-- save drivespec information
		tn = string.format("pctype%02d", dtype)
		drivespecs[tn] = {}
		drivespecs[tn]['friendlyname']	= string.format("Winchester: PC type %d, CHS %d:%d:17, %d MiB",
											dtype, ncyls, nheads, mbytes)
		drivespecs[tn]['steprate']		= 0.250		-- default step rate: 250us/step, buffered seek
		drivespecs[tn]['spinup']		= 0			-- no spinup time (the drive signals READY)
		drivespecs[tn]['tracks']		= ncyls		-- number of cylinders
		drivespecs[tn]['heads']			= nheads	-- number of heads
		drivespecs[tn]['tpi']			= 0			-- tracks per inch, unknown, "do not enforce" (but issue a warning in the log)
	end
end

-- Run the initialisation function
init()

--[[
Given the drive type, track, head and sector, return a list of output pins which need to be set.
Called once per sector on hard-sectored media, once per track on soft-sectored media
--]]
function getDriveOutputs(drivetype, track, head, sector)
	pins = 0

	-- check the drive type for validity
	if drivespecs[drivetype] == nil then
		error("Unrecognised drive type '" .. drivetype .. "'.")
	end

	-- select the drive; assume it's jumpered for DS0
	pins = pins + PIN_DS0

	-- Handle side selection
	if head > drivespecs[drivetype]['heads'] then error("Head number " .. head .. " out of range.") end
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

