__update_table =
{
	"scpt/c-init.lua",
	"scpt/player.lua",
	"scpt/spells.lua",
	"scpt/s_aux.lua",
	"scpt/s_mana.lua",
	"scpt/s_fire.lua",
	"scpt/s_air.lua",
	"scpt/s_water.lua",
	"scpt/s_earth.lua",
	"scpt/s_convey.lua",
	"scpt/s_divin.lua",
	"scpt/s_tempo.lua",
	"scpt/s_meta.lua",
	"scpt/s_nature.lua",
	"scpt/s_udun.lua",
}

function update_client()
        local k, e

        for k, e in __update_table do
                remote_update_lua(Ind, e)
        end
end
