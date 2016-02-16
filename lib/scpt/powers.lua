__powers = {}
__powers_max = 0

function add_special_power(t)
	assert(t.name, "no power name")
	assert(t.desc, "no power desc")
	assert(t.exec, "no power exec")
	if not t.condition then t.condition = function() end end

	__powers[__powers_max] = t
	__powers_max = __powers_max + 1
end

add_special_power {
	["name"] = 	"Worldportation",
	["desc"] = {
			"Allows you travel between worlds at will by using",
			"the Jumpgates. Halves your HP and mana when used."
	},
	["condition"] = function()
	end,
	["exec"] = 	function()
	end,
}
add_special_power {
	["name"] = 	"Wildportation",
	["desc"] = {
			"Allows you extremely fast travel in the wilderness by using",
			"the Jumpgates. Uses 10 mana to acticate."
	},
	["condition"] = function()
	end,
	["exec"] = 	function()
	end,
}
add_special_power {
	["name"] = 	"Alchemy",
	["desc"] = {
			"Allows you extremely fast travel in the wilderness by using",
			"the Jumpgates. Uses 10 mana to acticate."
	},
	["condition"] = function()
	end,
	["exec"] = 	function()
	end,
}
