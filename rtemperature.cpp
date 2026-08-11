// rtemperature - a Deskbar tray thermometer for Haiku.
//
// It shows the machine's temperature in the Deskbar tray (the replicant shelf,
// next to the clock) and nowhere else: no window, and no entry in the Deskbar
// team list. The executable adds a replicant to the tray and then quits, so
// the reading lives inside Deskbar's own process and nothing is left in the
// task list. Deskbar persists its shelf, so the reading reappears on its own
// after a reboot - that is the "shown at startup" behaviour, for free, as long
// as this executable stays where it is.
//
// The temperature comes from the ACPI thermal zone the kernel publishes at
// /dev/power/acpi_thermal/N. That device returns a short text report, e.g.
//
//     Critical Temperature: 100.0 C
//     Current Temperature: 64.0 C
//
// so this parses the "Current Temperature" line. The read is one-shot and
// non-blocking: the device blocks a second read, which would freeze Deskbar's
// window thread, so it is read exactly once per refresh.
//
// IMPORTANT - build with the PRIMARY architecture. On an x86 hybrid Haiku,
// Deskbar is the x86_gcc2 binary and can only load add-ons of the same
// architecture, so this must be built with the gcc2 compiler (the default
// `g++`). Building it with the modern gcc makes Deskbar crash when it tries to
// load the replicant. The code is kept gcc2-compatible for that reason.
//
// Build:  make
// Use:    ./rtemperature            add it to the tray
//         ./rtemperature --remove   take it back out
//         ./rtemperature --print    print the current reading and exit
//
// This file is both the application (main) and the Deskbar add-on that Deskbar
// loads to re-create the replicant (instantiate_deskbar_item).

#include <Application.h>
#include <Deskbar.h>
#include <MessageRunner.h>
#include <Roster.h>
#include <View.h>
#include <String.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define APP_SIGNATURE	"application/x-vnd.rain-rtemperature"
#define REPLICANT_NAME	"rtemperature"

static const uint32 kMsgTick = 'tick';
static const bigtime_t kInterval = 3000000;		// refresh every 3 seconds


// Read one ACPI thermal zone and parse the "Current Temperature" line.
// Returns true and fills _celsius on success.
static bool
ReadTemperature(float* _celsius)
{
	for (int zone = 0; zone < 4; zone++) {
		char path[64];
		snprintf(path, sizeof(path), "/dev/power/acpi_thermal/%d", zone);

		// Non-blocking, single read: a second read on this device blocks.
		int fd = open(path, O_RDONLY | O_NONBLOCK);
		if (fd < 0)
			continue;

		char buf[256];
		ssize_t got = read(fd, buf, sizeof(buf) - 1);
		close(fd);
		if (got <= 0)
			continue;
		buf[got] = '\0';

		const char* p = strstr(buf, "Current Temperature:");
		if (p == NULL)
			continue;
		p += strlen("Current Temperature:");
		float value = (float)atof(p);
		if (value > 0.0f && value < 200.0f) {
			*_celsius = value;
			return true;
		}
	}
	return false;
}


class TempView : public BView {
public:
						TempView(BRect frame);
						TempView(BMessage* archive);
	virtual				~TempView();

	static BArchivable*	Instantiate(BMessage* archive);
	virtual status_t	Archive(BMessage* archive, bool deep = true) const;

	virtual void		AttachedToWindow();
	virtual void		DetachedFromWindow();
	virtual void		Draw(BRect updateRect);
	virtual void		MessageReceived(BMessage* message);
	virtual void		GetPreferredSize(float* width, float* height);

private:
	void				Refresh();
	void				DrawThermometer(BRect bounds);

	bool				fValid;
	float				fCelsius;
	BMessageRunner*		fRunner;
};


TempView::TempView(BRect frame)
	:
	BView(frame, REPLICANT_NAME, B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS),
	fValid(false),
	fCelsius(0.0f),
	fRunner(NULL)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}


TempView::TempView(BMessage* archive)
	:
	BView(archive),
	fValid(false),
	fCelsius(0.0f),
	fRunner(NULL)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}


TempView::~TempView()
{
	delete fRunner;
}


BArchivable*
TempView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, REPLICANT_NAME))
		return NULL;
	return new TempView(archive);
}


status_t
TempView::Archive(BMessage* archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);
	if (status == B_OK)
		status = archive->AddString("add_on", APP_SIGNATURE);
	if (status == B_OK)
		status = archive->AddString("class", REPLICANT_NAME);
	return status;
}


void
TempView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (Parent() != NULL)
		SetLowColor(Parent()->ViewColor());
	else
		SetLowColor(ui_color(B_MENU_BACKGROUND_COLOR));

	BFont font;
	GetFont(&font);
	font.SetSize(11.0f);
	font.SetFace(B_BOLD_FACE);
	SetFont(&font);

	Refresh();

	BMessage tick(kMsgTick);
	fRunner = new BMessageRunner(BMessenger(this), &tick, kInterval);
}


void
TempView::DetachedFromWindow()
{
	delete fRunner;
	fRunner = NULL;
}


void
TempView::MessageReceived(BMessage* message)
{
	if (message->what == kMsgTick) {
		Refresh();
		return;
	}
	BView::MessageReceived(message);
}


void
TempView::Refresh()
{
	float now = 0.0f;
	bool valid = ReadTemperature(&now);
	if (valid != fValid || now != fCelsius) {
		fValid = valid;
		fCelsius = now;
		Invalidate();
	}
}


void
TempView::GetPreferredSize(float* width, float* height)
{
	if (width != NULL)
		*width = 41.0f;
	if (height != NULL)
		*height = 15.0f;
}


// A small BeOS-style thermometer: a white tube with a red bulb, the mercury
// column rising with temperature. Drawn on the left; the number sits to the
// right of it.
void
TempView::DrawThermometer(BRect bounds)
{
	const rgb_color black = make_color(40, 40, 40);
	const rgb_color white = make_color(245, 245, 245);
	rgb_color mercury = make_color(210, 40, 40);
	if (fValid && fCelsius >= 80.0f)
		mercury = make_color(230, 20, 20);
	else if (fValid && fCelsius < 45.0f)
		mercury = make_color(210, 90, 60);

	float h = bounds.Height();
	float bulbR = 2.6f;
	float cx = 4.0f;
	float bulbCY = h - bulbR - 1.0f;
	float tubeW = 3.0f;
	float tubeLeft = cx - tubeW / 2.0f;
	float tubeTop = 1.5f;
	float tubeBottom = bulbCY;

	SetDrawingMode(B_OP_OVER);
	SetPenSize(1.0f);

	// The tube: white body, dark outline.
	BRect tube(tubeLeft, tubeTop, tubeLeft + tubeW, tubeBottom);
	SetHighColor(white);
	FillRoundRect(tube, 1.4f, 1.4f);

	// Mercury column, scaled 15..95 C over the tube's length.
	if (fValid) {
		float frac = (fCelsius - 15.0f) / (95.0f - 15.0f);
		if (frac < 0.06f)
			frac = 0.06f;
		if (frac > 1.0f)
			frac = 1.0f;
		float top = tubeTop + (1.0f - frac) * (tubeBottom - tubeTop);
		SetHighColor(mercury);
		FillRect(BRect(tubeLeft + 0.5f, top, tubeLeft + tubeW - 0.5f,
			tubeBottom));
	}

	SetHighColor(black);
	StrokeRoundRect(tube, 1.4f, 1.4f);

	// The bulb: always full of mercury.
	BPoint center(cx, bulbCY);
	SetHighColor(mercury);
	FillEllipse(center, bulbR, bulbR);
	SetHighColor(black);
	StrokeEllipse(center, bulbR, bulbR);
}


void
TempView::Draw(BRect updateRect)
{
	FillRect(updateRect, B_SOLID_LOW);

	BRect bounds = Bounds();
	DrawThermometer(bounds);

	char text[32];
	if (!fValid) {
		strcpy(text, "--");
	} else {
		int whole = (int)(fCelsius + 0.5f);
		snprintf(text, sizeof(text), "%d\xc2\xb0" "C", whole);
	}

	// The number stays in the menu text colour whatever the temperature is;
	// only the thermometer's mercury changes colour.
	SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));
	SetDrawingMode(B_OP_OVER);

	font_height fh;
	GetFontHeight(&fh);
	float x = 8.0f;								// directly beside thermometer
	float y = (bounds.Height() - (fh.ascent + fh.descent)) / 2 + fh.ascent;

	DrawString(text, BPoint(x, y));
}


// ---- the Deskbar add-on entry point --------------------------------------
// Deskbar calls this to (re-)create the replicant, including after a reboot.

extern "C" _EXPORT BView* instantiate_deskbar_item(float maxWidth,
	float maxHeight);

extern "C" _EXPORT BView*
instantiate_deskbar_item(float maxWidth, float maxHeight)
{
	(void)maxWidth;
	(void)maxHeight;
	return new TempView(BRect(0, 0, 40, 14));
}


// ---- the application: add the replicant, then get out of the way ----------

class App : public BApplication {
public:
	App()
		:
		BApplication(APP_SIGNATURE),
		fRemove(false)
	{
	}

	void SetRemove(bool remove) { fRemove = remove; }

	virtual void ReadyToRun()
	{
		BDeskbar deskbar;

		if (deskbar.HasItem(REPLICANT_NAME))
			deskbar.RemoveItem(REPLICANT_NAME);

		if (!fRemove) {
			app_info info;
			if (be_app->GetAppInfo(&info) == B_OK) {
				int32 id;
				if (deskbar.AddItem(&info.ref, &id) != B_OK)
					fprintf(stderr, "could not add the tray item\n");
			}
		}
		Quit();
	}

private:
	bool fRemove;
};


int
main(int argc, char** argv)
{
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--print") == 0) {
			float c = 0.0f;
			if (ReadTemperature(&c))
				printf("%.1f C\n", c);
			else
				printf("no readable thermal zone\n");
			return 0;
		}
	}

	App app;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--remove") == 0)
			app.SetRemove(true);
	}
	app.Run();
	return 0;
}
