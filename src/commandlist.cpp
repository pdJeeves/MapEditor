#include "commandlist.h"
#include "mainwindow.h"

AddRoomCommand::AddRoomCommand(int i, const Room &room) :
	i(i),
	room(room)
{
}

void AddRoomCommand::rollBack(MainWindow * window)
{
	window->rooms[i].remove(room);
}

void AddRoomCommand::rollForward(MainWindow * window)
{
	window->rooms[i].push_back(room);
}

void AggregateCommand::initialize(MainWindow * window)
{
	for(auto i = begin(); i != end(); ++i)
	{
		(*i)->initialize(window);
	}
}

void AggregateCommand::rollForward(MainWindow * window)
{
	for(auto i = begin(); i != end(); ++i)
	{
		(*i)->rollForward(window);
	}
}

void AggregateCommand::rollBack(MainWindow * window)
{
	for(auto i = begin(); i != end(); ++i)
	{
		(*i)->rollBack(window);
	}
}

CommandList::CommandList()
{
	itr = list.end();
}

bool CommandList::canRollBack()    const
{
	return itr != list.begin();
}

bool CommandList::canRollForward() const
{
	return itr != list.end();
}

void CommandList::rollBack(MainWindow * window)
{
	if(itr != list.begin())
	{
		--itr;
		(*itr)->rollBack(window);
	}
}

void CommandList::rollForward(MainWindow * window)
{
	if(itr != list.end())
	{
		(*itr)->rollForward(window);
		++itr;
	}
}

void CommandList::push(CommandInterface * it, MainWindow * window)
{
	if(itr != list.end())
	{
		list.erase(itr, list.end());
	}

	list.emplace_back(it);
	itr = list.end();
	it->initialize(window);
}
