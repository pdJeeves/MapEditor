#ifndef COMMANDLIST_H
#define COMMANDLIST_H
#include "room.h"
#include <memory>
#include <list>

class MainWindow;

struct CommandInterface
{
	virtual ~CommandInterface() {}
	virtual void rollBack(MainWindow * window) = 0;
	virtual void rollForward(MainWindow * window) = 0;
	virtual void initialize(MainWindow * window) { rollForward(window); }
};


class AddRoomCommand : public CommandInterface
{
protected:
	const int i;
	const Room room;

public:
	AddRoomCommand(int i, const Room & room);

	void rollBack(MainWindow * window) override;
	void rollForward(MainWindow * window) override;
};

class RemoveRoomCommand : public AddRoomCommand
{
public:
typedef AddRoomCommand super;
	RemoveRoomCommand(int i, const Room & room) :
		super(i, room)
	{
	}
	void rollForward(MainWindow * window) override { super::rollBack(window); }
	void rollBack(MainWindow * window)    override { super::rollForward(window); }
};

class AggregateCommand : public CommandInterface, public std::list<std::unique_ptr<CommandInterface> >
{
public:
	void initialize(MainWindow * window) override;
	void rollForward(MainWindow * window) override;
	void rollBack(MainWindow * window)    override;
};

class CommandList
{
typedef std::list<std::unique_ptr<CommandInterface> > List;
private:
	List		    list;
	List::iterator  itr;

public:
	CommandList();

	bool canRollBack()    const;
	bool canRollForward() const;

	void rollBack(MainWindow * window);
	void rollForward(MainWindow * window);

	void push(CommandInterface *, MainWindow *window);
};


#endif // COMMANDLIST_H
