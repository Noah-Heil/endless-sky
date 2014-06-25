/* TradingPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "TradingPanel.h"

#include "Color.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "MapPanel.h"
#include "UI.h"

#include <string>

using namespace std;

namespace {
	const string TRADE_LEVEL[] = {
		"(very low)",
		"(low)",
		"(medium)",
		"(high)",
		"(very high)"
	};
	
	static const int MIN_X = -310;
	static const int MAX_X = 190;
	
	static const int NAME_X = -290;
	static const int PRICE_X = -150;
	static const int LEVEL_X = -110;
	static const int BUY_X = 0;
	static const int SELL_X = 60;
	static const int HOLD_X = 120;
	
	static const int FIRST_Y = 80;
}



TradingPanel::TradingPanel(const GameData &data, PlayerInfo &player)
	: data(data), player(player),
	system(*player.GetSystem()), selectedRow(0)
{
	SetTrapAllEvents(false);
}


	
void TradingPanel::Draw() const
{
	Color back(.1, .1);
	FillShader::Fill(Point(-60., FIRST_Y + 20 * selectedRow + 33), Point(480., 20.), back);
	
	const Font &font = FontSet::Get(14);
	Color unselected(.5, 1.);
	Color selected(.8, 1.);
	
	int y = FIRST_Y;
	FillShader::Fill(Point(-60., y + 15.), Point(480., 1.), unselected);
	
	font.Draw("Commodity", Point(NAME_X, y), selected);
	font.Draw("Price", Point(PRICE_X, y), selected);
	
	string mod = "x " + to_string(Modifier());
	font.Draw(mod, Point(BUY_X, y), unselected);
	font.Draw(mod, Point(SELL_X, y), unselected);
	
	font.Draw("In Hold", Point(HOLD_X, y), selected);
	
	y += 5;
	int lastY = y + 20 * data.Commodities().size() + 25;
	font.Draw("free:", Point(SELL_X + 5, lastY), selected);
	font.Draw(to_string(player.Cargo().Free()), Point(HOLD_X, lastY), selected);
	
	int outfits = player.Cargo().OutfitsSize();
	if(outfits)
	{
		string str = to_string(outfits) + " tons of plundered outfits.";
		font.Draw(str, Point(NAME_X, lastY), unselected);
	}
	
	int i = 0;
	for(const Trade::Commodity &commodity : data.Commodities())
	{
		y += 20;
		int price = system.Trade(commodity.name);
		
		const Color &color = (i++ == selectedRow ? selected : unselected);
		font.Draw(commodity.name, Point(NAME_X, y), color);
		font.Draw(to_string(price), Point(PRICE_X, y), color);
		
		int level = (price - commodity.low);
		if(level < 0)
			level = 0;
		else if(level >= (commodity.high - commodity.low))
			level = 4;
		else
			level = (5 * level) / (commodity.high - commodity.low);
		font.Draw(TRADE_LEVEL[level], Point(LEVEL_X, y), color);
		
		font.Draw("[buy]", Point(BUY_X, y), color);
		font.Draw("[sell]", Point(SELL_X, y), color);
		
		int hold = player.Cargo().Get(commodity.name);
		if(hold)
			font.Draw(to_string(hold), Point(HOLD_X, y), selected);
	}
}



// Only override the ones you need; the default action is to return false.
bool TradingPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if(key == SDLK_UP && selectedRow)
		--selectedRow;
	else if(key == SDLK_DOWN && selectedRow < static_cast<int>(data.Commodities().size()) - 1)
		++selectedRow;
	else if(key == '=' || key == SDLK_RETURN || key == SDLK_SPACE)
		Buy(1);
	else if(key == '-' || key == SDLK_BACKSPACE || key == SDLK_DELETE)
		Buy(-1);
	else if(key == data.Keys().Get(Key::MAP))
		GetUI()->Push(new MapPanel(data, player, selectedRow));
	else
		return false;
	
	return true;
}



bool TradingPanel::Click(int x, int y)
{
	int maxY = FIRST_Y + 25 + 20 * data.Commodities().size();
	if(x >= MIN_X && x <= MAX_X && y >= FIRST_Y + 25 && y < maxY)
	{
		selectedRow = (y - FIRST_Y - 25) / 20;
		if(x >= BUY_X && x < SELL_X)
			Buy(1);
		else if(x >= SELL_X && x < HOLD_X)
			Buy(-1);
	}
	else
		return false;
	
	return true;
}



void TradingPanel::Buy(int amount)
{
	amount *= Modifier();
	const string &type = data.Commodities()[selectedRow].name;
	int price = system.Trade(type);
	
	amount = min(amount, player.Accounts().Credits() / price);
	amount = player.Cargo().Transfer(type, -amount);
	player.Accounts().AddCredits(amount * price);
}



int TradingPanel::Modifier()
{
	SDL_Keymod mod = SDL_GetModState();
	
	int modifier = 1;
	if(mod & KMOD_CTRL)
		modifier *= 20;
	if(mod & KMOD_SHIFT)
		modifier *= 5;
	
	return modifier;
}
