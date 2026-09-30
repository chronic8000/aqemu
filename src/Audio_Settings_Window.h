/****************************************************************************
** This file is part of AQEMU.
****************************************************************************/

#ifndef AUDIO_SETTINGS_WINDOW_H
#define AUDIO_SETTINGS_WINDOW_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QSpinBox;
class QLineEdit;
class QWidget;

/** File → Audio (host defaults) or VM → Audio (one machine's override). */
class Audio_Settings_Window: public QDialog
{
	Q_OBJECT

	public:
		explicit Audio_Settings_Window( bool host_defaults, QWidget *parent = NULL );
		/** VM override. options_blob and virtio counts are read and written. */
		void Set_VM( bool use_host_defaults, const QString &options_blob,
			int jacks, int streams, int chmaps, const QString &qemu_binary );
		bool Use_Host_Defaults() const;
		QString Options_Blob() const;
		int VirtIO_Jacks() const;
		int VirtIO_Streams() const;
		int VirtIO_Chmaps() const;

	private slots:
		void Apply_Backend_Page();
		void Refresh_Devices();
		void Save_And_Accept();

	private:
		void Load_Profile( bool from_host );
		void Fill_Combo( QComboBox *box, const QString &backend, bool capture, const QString &current );

		bool Host_Mode;
		QString Qemu_Binary;
		QCheckBox *Use_Defaults;
		QComboBox *Backend;
		QComboBox *Out_Device;
		QComboBox *In_Device;
		QSpinBox *Out_Voices;
		QSpinBox *In_Voices;
		QSpinBox *Timer_Period;
		QSpinBox *Out_Frequency;
		QSpinBox *In_Frequency;
		QSpinBox *Out_Channels;
		QSpinBox *In_Channels;
		QComboBox *Out_Format;
		QComboBox *In_Format;
		QSpinBox *Out_Buffer;
		QSpinBox *In_Buffer;
		QComboBox *Out_Mix;
		QComboBox *In_Mix;
		QComboBox *Out_Fixed;
		QComboBox *In_Fixed;
		QWidget *Detail;
		QSpinBox *Latency;
		QSpinBox *Out_Buffer_Count;
		QSpinBox *In_Buffer_Count;
		QLineEdit *Server;
		QLineEdit *Out_Stream;
		QLineEdit *In_Stream;
		QSpinBox *Out_Latency;
		QSpinBox *In_Latency;
		QSpinBox *Out_Period;
		QSpinBox *In_Period;
		QComboBox *Out_Poll;
		QComboBox *In_Poll;
		QSpinBox *Threshold;
		QLineEdit *Wav_Path;
		QSpinBox *Nsamp;
		QLineEdit *Jack_Server;
		QLineEdit *Jack_Client;
		QSpinBox *Virt_Jacks;
		QSpinBox *Virt_Streams;
		QSpinBox *Virt_Chmaps;
		QWidget *Virt_Box;
		QString Saved_Blob;
};

#endif
