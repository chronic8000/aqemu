#ifndef APPLE_SOC_BUTTON_PAD_H
#define APPLE_SOC_BUTTON_PAD_H

#include <QWidget>

class QToolButton;
class QTimer;

/** Neutral floating control pad for Inferno hardware buttons (not an iPhone bezel). */
class Apple_SoC_Button_Pad : public QWidget
{
	Q_OBJECT
public:
	explicit Apple_SoC_Button_Pad( QWidget *parent = nullptr );

signals:
	void Home_Clicked();
	void Home_Double_Clicked();
	void Power_Clicked();
	void Power_Hold();
	void Vol_Down();
	void Vol_Up();
	void SOS_Triggered();

protected:
	bool eventFilter( QObject *obj, QEvent *event ) override;

private:
	QToolButton *Btn_Home;
	QToolButton *Btn_Power;
	QToolButton *Btn_Vol_Down;
	QToolButton *Btn_Vol_Up;
	QToolButton *Btn_SOS;
	QTimer *Home_Click_Timer;
	bool Home_Click_Pending;
};

#endif
