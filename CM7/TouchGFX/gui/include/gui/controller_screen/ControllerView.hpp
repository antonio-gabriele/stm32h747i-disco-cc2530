#ifndef CONTROLLERVIEW_HPP
#define CONTROLLERVIEW_HPP

#include <gui_generated/controller_screen/ControllerViewBase.hpp>
#include <gui/controller_screen/ControllerPresenter.hpp>

class ControllerView : public ControllerViewBase
{
public:
    ControllerView();
    virtual ~ControllerView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void deviceScrollListUpdateItem(Device&, int16_t);
    virtual void tick();
protected:
private:
    bool flg1;
};

#endif // CONTROLLERVIEW_HPP
