#ifndef ALTERSRC_ITOOLGUNMODEDISPLAY_H
#define ALTERSRC_ITOOLGUNMODEDISPLAY_H

// IMyPanel.h
class IToolGunModeDisplay
{
public:
	virtual void		Create( vgui::VPANEL parent ) = 0;
	virtual void		Destroy( void ) = 0;
};

extern IToolGunModeDisplay* toolgunmodedisplay;

#endif // !ALTERSRC_ITOOLGUNMODEDISPLAY_H