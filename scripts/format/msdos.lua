--[[
#############################
# DiscFerret Format Specification File
#
# 40/80 track generic, soft sectored
#############################
]]

formatspec_version = 1.0

formatspecs = {
	gen40 = {
		-- format name
		friendlyname	= "40 track, soft-sectored, generic",
		-- minimum track number
		mintrack		= 0,
		-- maximum track number
		maxtrack		= 39,
		-- track stepping -- 1=singlestep, 2=doublestep
		trackstep		= 1,
		-- minimum head number
		minhead			= 0,
		-- maximum head number
		maxhead			= 1,
		-- sectoring; 0=soft-sectored, or number of sectors if hard-sectored
		sectors			= 0,
	},

	gen80ds = {
		-- format name
		name			= "80 track, double-sided, soft-sectored, generic",
		-- minimum track number
		mintrack		= 0,
		-- maximum track number
		maxtrack		= 79,
		-- track stepping -- 1=singlestep, 2=doublestep
		trackstep		= 1,
		-- minimum head number
		minhead			= 0,
		-- maximum head number
		maxhead			= 1,
		-- sectoring; 0=soft-sectored
		sectors			= 0,
	}
}

