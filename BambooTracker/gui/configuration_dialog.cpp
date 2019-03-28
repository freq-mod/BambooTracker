#include "configuration_dialog.hpp"
#include "ui_configuration_dialog.h"
#include <algorithm>
#include <functional>
#include <QPushButton>
#include <QMenu>
#include <QAudio>
#include <QAudioDeviceInfo>
#include "slider_style.hpp"
#include "fm_envelope_set_edit_dialog.hpp"
#include "midi/midi.hpp"

ConfigurationDialog::ConfigurationDialog(std::weak_ptr<Configuration> config, QWidget *parent)
	: QDialog(parent),
	  ui(new Ui::ConfigurationDialog),
	  config_(config)
{
	ui->setupUi(this);

	setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);
	QObject::connect(ui->buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
					 this, [&] {
		on_ConfigurationDialog_accepted();
		emit applyPressed();
	});

	// General //
	// General settings
	ui->generalSettingsListWidget->item(0)->setCheckState(toCheckState(config.lock()->getWarpCursor()));
	ui->generalSettingsListWidget->item(1)->setCheckState(toCheckState(config.lock()->getWarpAcrossOrders()));
	ui->generalSettingsListWidget->item(2)->setCheckState(toCheckState(config.lock()->getShowRowNumberInHex()));
	ui->generalSettingsListWidget->item(3)->setCheckState(toCheckState(config.lock()->getShowPreviousNextOrders()));
	ui->generalSettingsListWidget->item(4)->setCheckState(toCheckState(config.lock()->getBackupModules()));
	ui->generalSettingsListWidget->item(5)->setCheckState(toCheckState(config.lock()->getDontSelectOnDoubleClick()));
	ui->generalSettingsListWidget->item(6)->setCheckState(toCheckState(config.lock()->getReverseFMVolumeOrder()));
	ui->generalSettingsListWidget->item(7)->setCheckState(toCheckState(config.lock()->getMoveCursorToRight()));
	ui->generalSettingsListWidget->item(8)->setCheckState(toCheckState(config.lock()->getRetrieveChannelState()));
	ui->generalSettingsListWidget->item(9)->setCheckState(toCheckState(config.lock()->getEnableTranslation()));

	// Edit settings
	ui->pageJumpLengthSpinBox->setValue(static_cast<int>(config.lock()->getPageJumpLength()));

	// Keys
	ui->keyOffKeySequenceEdit->setKeySequence(
				QString::fromUtf8(config.lock()->getKeyOffKey().c_str(),
								  static_cast<int>(config.lock()->getKeyOffKey().length())));
	ui->octaveUpKeySequenceEdit->setKeySequence(
				QString::fromUtf8(config.lock()->getOctaveUpKey().c_str(),
								  static_cast<int>(config.lock()->getOctaveUpKey().length())));
	ui->octaveDownKeySequenceEdit->setKeySequence(
				QString::fromUtf8(config.lock()->getOctaveDownKey().c_str(),
								  static_cast<int>(config.lock()->getOctaveDownKey().length())));
	ui->echoBufferKeySequenceEdit->setKeySequence(
				QString::fromUtf8(config.lock()->getEchoBufferKey().c_str(),
								  static_cast<int>(config.lock()->getEchoBufferKey().length())));
	ui->keyboardTypeComboBox->setCurrentIndex(static_cast<int>(config.lock()->getNoteEntryLayout()));

	// Sound //
	int devRow = -1;
	int defDevRow = 0;
	for (auto& info : QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
		ui->soundDeviceComboBox->addItem(info.deviceName());
		if (info.deviceName() == QString::fromUtf8(config.lock()->getSoundDevice().c_str(),
												   static_cast<int>(config.lock()->getSoundDevice().length())))
			devRow = ui->soundDeviceComboBox->count() - 1;
		if (info.deviceName() == QAudioDeviceInfo::defaultOutputDevice().deviceName()) {
			defDevRow = ui->soundDeviceComboBox->count() - 1;
		}
	}
	ui->soundDeviceComboBox->setCurrentIndex((devRow == -1) ? defDevRow : devRow);

	ui->useSCCICheckBox->setCheckState(config.lock()->getUseSCCI() ? Qt::Checked : Qt::Unchecked);
	ui->sampleRateComboBox->addItem("44100Hz", 44100);
	ui->sampleRateComboBox->addItem("48000Hz", 48000);
	ui->sampleRateComboBox->addItem("55466Hz", 55466);
	switch (config.lock()->getSampleRate()) {
	case 44100:	ui->sampleRateComboBox->setCurrentIndex(0);	break;
	case 48000:	ui->sampleRateComboBox->setCurrentIndex(1);	break;
	case 55466:	ui->sampleRateComboBox->setCurrentIndex(2);	break;
	}
	ui->bufferLengthHorizontalSlider->setStyle(new SliderStyle());
	QObject::connect(ui->bufferLengthHorizontalSlider, &QSlider::valueChanged,
					 this, [&](int value) {
		ui->bufferLengthLabel->setText(QString::number(value) + "ms");
	});
	ui->bufferLengthHorizontalSlider->setValue(static_cast<int>(config.lock()->getBufferLength()));

	// Midi //
	MidiInterface &midiIntf = MidiInterface::instance();
	if (midiIntf.supportsVirtualPort())
		ui->midiInputNameLine->setPlaceholderText(tr("Virtual port"));
	ui->midiInputNameLine->setText(QString::fromStdString(config.lock()->getMidiInputPort()));

	// Mixer //
	ui->masterMixerSlider->setText(tr("Master"));
	ui->masterMixerSlider->setSuffix("%");
	ui->masterMixerSlider->setMaximum(100);
	ui->masterMixerSlider->setMinimum(0);
	ui->masterMixerSlider->setTickPosition(QSlider::TicksBothSides);
	ui->masterMixerSlider->setTickInterval(20);
	ui->masterMixerSlider->setValue(config.lock()->getMixerVolumeMaster());

	ui->fmMixerSlider->setText("FM");
	ui->fmMixerSlider->setSuffix("dB");
	ui->fmMixerSlider->setMaximum(120);
	ui->fmMixerSlider->setMinimum(-120);
	ui->fmMixerSlider->setValueRate(0.1);
	ui->fmMixerSlider->setSign(true);
	ui->fmMixerSlider->setTickPosition(QSlider::TicksBothSides);
	ui->fmMixerSlider->setTickInterval(20);
	ui->fmMixerSlider->setValue(static_cast<int>(config.lock()->getMixerVolumeFM() * 10));

	ui->ssgMixerSlider->setText("SSG");
	ui->ssgMixerSlider->setSuffix("dB");
	ui->ssgMixerSlider->setMaximum(120);
	ui->ssgMixerSlider->setMinimum(-120);
	ui->ssgMixerSlider->setValueRate(0.1);
	ui->ssgMixerSlider->setSign(true);
	ui->ssgMixerSlider->setTickPosition(QSlider::TicksBothSides);
	ui->ssgMixerSlider->setTickInterval(20);
	ui->ssgMixerSlider->setValue(static_cast<int>(config.lock()->getMixerVolumeSSG() * 10));

	// Input //
	fmEnvelopeTexts_ = config.lock()->getFMEnvelopeTexts();
	updateEnvelopeSetUi();
}

ConfigurationDialog::~ConfigurationDialog()
{
	delete ui;
}

void ConfigurationDialog::on_ConfigurationDialog_accepted()
{
	// General //
	// General settings
	config_.lock()->setWarpCursor(fromCheckState(ui->generalSettingsListWidget->item(0)->checkState()));
	config_.lock()->setWarpAcrossOrders(fromCheckState(ui->generalSettingsListWidget->item(1)->checkState()));
	config_.lock()->setShowRowNumberInHex(fromCheckState(ui->generalSettingsListWidget->item(2)->checkState()));
	config_.lock()->setShowPreviousNextOrders(fromCheckState(ui->generalSettingsListWidget->item(3)->checkState()));
	config_.lock()->setBackupModules(fromCheckState(ui->generalSettingsListWidget->item(4)->checkState()));
	config_.lock()->setDontSelectOnDoubleClick(fromCheckState(ui->generalSettingsListWidget->item(5)->checkState()));
	config_.lock()->setReverseFMVolumeOrder(fromCheckState(ui->generalSettingsListWidget->item(6)->checkState()));
	config_.lock()->setMoveCursorToRight(fromCheckState(ui->generalSettingsListWidget->item(7)->checkState()));
	config_.lock()->setRetrieveChannelState(fromCheckState(ui->generalSettingsListWidget->item(8)->checkState()));
	config_.lock()->setEnableTranslation(fromCheckState(ui->generalSettingsListWidget->item(9)->checkState()));

	// Edit settings
	config_.lock()->setPageJumpLength(static_cast<size_t>(ui->pageJumpLengthSpinBox->value()));

	// Keys
	config_.lock()->setKeyOffKey(ui->keyOffKeySequenceEdit->keySequence().toString().toStdString());
	config_.lock()->setOctaveUpKey(ui->octaveUpKeySequenceEdit->keySequence().toString().toStdString());
	config_.lock()->setOctaveDownKey(ui->octaveDownKeySequenceEdit->keySequence().toString().toStdString());
	config_.lock()->setEchoBufferKey(ui->echoBufferKeySequenceEdit->keySequence().toString().toStdString());
	config_.lock()->setNoteEntryLayout(static_cast<Configuration::KeyboardLayout>(ui->keyboardTypeComboBox->currentIndex()));

	// Sound //
	config_.lock()->setSoundDevice(ui->soundDeviceComboBox->currentText().toUtf8().toStdString());
	config_.lock()->setUseSCCI(ui->useSCCICheckBox->checkState() == Qt::Checked);
	config_.lock()->setSampleRate(ui->sampleRateComboBox->currentData(Qt::UserRole).toUInt());
	config_.lock()->setBufferLength(static_cast<size_t>(ui->bufferLengthHorizontalSlider->value()));

	// Midi //
	config_.lock()->setMidiInputPort(ui->midiInputNameLine->text().toStdString());

	// Mixer //
	config_.lock()->setMixerVolumeMaster(ui->masterMixerSlider->value());
	config_.lock()->setMixerVolumeFM(ui->fmMixerSlider->value() * 0.1);
	config_.lock()->setMixerVolumeSSG(ui->ssgMixerSlider->value() * 0.1);

	// Input //
	std::sort(fmEnvelopeTexts_.begin(), fmEnvelopeTexts_.end(),
			  [](const FMEnvelopeText& a, const FMEnvelopeText& b) -> bool { return (a.name < b.name); });
	config_.lock()->setFMEnvelopeTexts(fmEnvelopeTexts_);
}

/***** General *****/
void ConfigurationDialog::on_generalSettingsListWidget_itemSelectionChanged()
{
	QString text;
	switch (ui->generalSettingsListWidget->currentRow()) {
	case 0:		// Warp cursor
		text = tr("Warp the cursor around the edges of the pattern editor.");
		break;
	case 1:		// Warp across orders
		text = tr("Move to previous or next order when reaching top or bottom in the pattern editor.");
		break;
	case 2:		// Show row numbers in hex
		text = tr("Display order numbers and the order count on the status bar in hexadecimal.");
		break;
	case 3:		// Preview previous/next orders
		text = tr("Preview previous and next orders in the pattern editor.");
		break;
	case 4:		// Backup modeles
		text = tr("Create a backup copy of the existing file when saving a module.");
		break;
	case 5:		// Don't select on double click
		text = tr("Don't select the whole track when double-clicking in the pattern editor.");
		break;
	case 6:		// Reverse FM volume order
		text = tr("Reverse the order of FM volume so that 00 is the quietest in the pattern editor.");
		break;
	case 7:		// Move cursor to right
		text = tr("Move the cursor to right after entering effects in the pattern editor.");
		break;
	case 8:		// Retrieve channel state
		text = tr("Reconstruct the current channel's state from previous orders upon playing.");
		break;
	case 9:		// Enable translation
		text = tr("Translate to your language from the next launch. "
				  "See readme to check supported languages.");
		break;
	default:
		text = "";
		break;
	}
	ui->descPlainTextEdit->setPlainText(tr("Description: ") + text);
}

/***** Mixer *****/
void ConfigurationDialog::on_mixerResetPushButton_clicked()
{
	ui->fmMixerSlider->setValue(0);
	ui->ssgMixerSlider->setValue(0);
}

/***** MIDI *****/
void ConfigurationDialog::on_midiInputChoiceButton_clicked()
{
	QToolButton *button = ui->midiInputChoiceButton;
	QMenu menu;
	QAction *action;

	MidiInterface &intf = MidiInterface::instance();
	std::vector<std::string> ports = intf.getRealInputPorts();
	bool vport = intf.supportsVirtualPort();

	if (vport) {
		ui->midiInputNameLine->setPlaceholderText(tr("Virtual port"));
		action = menu.addAction(tr("Virtual port"));
		action->setData(QString());
	}

	for (unsigned i = 0, n = ports.size(); i < n; ++i) {
		if (i == 0 && vport)
			menu.addSeparator();
		QString portName = QString::fromStdString(ports[i]);
		action = menu.addAction(portName);
		action->setData(portName);
	}

	QAction *choice = menu.exec(button->mapToGlobal(button->rect().bottomLeft()));
	if (choice) {
		QString portName = choice->data().toString();
		ui->midiInputNameLine->setText(portName);
	}
}

/***** Input *****/
void ConfigurationDialog::updateEnvelopeSetUi()
{
	std::sort(fmEnvelopeTexts_.begin(), fmEnvelopeTexts_.end(),
			  [](const FMEnvelopeText& a, const FMEnvelopeText& b) -> bool { return (a.name < b.name); });

	ui->envelopeTypeListWidget->clear();
	for (auto& texts : fmEnvelopeTexts_)
		ui->envelopeTypeListWidget->addItem(
					QString::fromUtf8(texts.name.c_str(), static_cast<int>(texts.name.length())));
}

void ConfigurationDialog::on_addEnvelopeSetPushButton_clicked()
{
	auto name = QString("Set %1").arg(fmEnvelopeTexts_.size() + 1);
	fmEnvelopeTexts_.push_back({ name.toUtf8().toStdString(), std::vector<FMEnvelopeTextType>() });
	updateEnvelopeSetUi();
	for (int i = ui->envelopeTypeListWidget->count() - 1; i >= 0; --i) {
		if (ui->envelopeTypeListWidget->item(i)->text() == name) {
			ui->envelopeTypeListWidget->setCurrentRow(i);
			break;
		}
	}
}

void ConfigurationDialog::on_removeEnvelopeSetpushButton_clicked()
{
	fmEnvelopeTexts_.erase(fmEnvelopeTexts_.begin() + ui->envelopeTypeListWidget->currentRow());
	updateEnvelopeSetUi();
}

void ConfigurationDialog::on_editEnvelopeSetPushButton_clicked()
{
	size_t row = static_cast<size_t>(ui->envelopeTypeListWidget->currentRow());
	FMEnvelopeSetEditDialog diag(fmEnvelopeTexts_.at(row).texts);
	diag.setWindowTitle(diag.windowTitle() + ": " + ui->envelopeSetNameLineEdit->text());
	if (diag.exec() == QDialog::Accepted) {
		fmEnvelopeTexts_.at(row).texts = diag.getSet();
	}
}

void ConfigurationDialog::on_envelopeSetNameLineEdit_textChanged(const QString &arg1)
{
	fmEnvelopeTexts_.at(static_cast<size_t>(ui->envelopeTypeListWidget->currentRow())).name = arg1.toStdString();
	ui->envelopeTypeListWidget->currentItem()->setText(arg1);
}

void ConfigurationDialog::on_envelopeTypeListWidget_currentRowChanged(int currentRow)
{
	if (currentRow == -1) {
		ui->editEnvelopeSetPushButton->setEnabled(false);
		ui->removeEnvelopeSetpushButton->setEnabled(false);
		ui->envelopeSetNameLineEdit->setEnabled(false);
	}
	else {
		ui->editEnvelopeSetPushButton->setEnabled(true);
		ui->removeEnvelopeSetpushButton->setEnabled(true);
		ui->envelopeSetNameLineEdit->setEnabled(true);
		ui->envelopeSetNameLineEdit->setText(ui->envelopeTypeListWidget->item(currentRow)->text());
	}
}
