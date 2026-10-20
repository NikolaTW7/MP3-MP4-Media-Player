#pragma once

namespace MyMediaPlayer {

    using namespace System;
    using namespace System::Drawing;
    using namespace System::Windows::Forms;

    // Floating control bar for full screen. An owned top-level window, so it always
    // sits above mpv's video and never takes focus. Playback logic stays in the player.
    public ref class FullScreenOverlay : public System::Windows::Forms::Form
    {
    public:
        // Thin hand-drawn slider, sharp at any DPI. Value runs from 0.0 to 1.0.
        ref class SeekBar : public Control
        {
        public:
            event EventHandler^ ValueChanged;    // while clicking or dragging
            event EventHandler^ ValueCommitted;  // when the mouse button is released

            Color FillColor;

            // Volume bar only: snap to exactly 0 or 1 near the ends, and keep the grab
            // point when the thumb is pressed.
            bool SnapToEnds;

            SeekBar()
            {
                SetStyle(ControlStyles::Selectable, false);  // never takes keyboard focus
                SetStyle(ControlStyles::UserPaint | ControlStyles::AllPaintingInWmPaint |
                         ControlStyles::OptimizedDoubleBuffer | ControlStyles::ResizeRedraw, true);
                FillColor = Color::FromArgb(230, 40, 40);
                this->Cursor = Cursors::Hand;
            }

            // Updates from the player are ignored mid-drag, so the thumb doesn't jump back.
            property double Value
            {
                double get() { return value; }
                void set(double v)
                {
                    if (dragging) return;
                    v = Math::Max(0.0, Math::Min(1.0, v));
                    if (v != value) { value = v; Invalidate(); }
                }
            }

            property bool IsDragging
            {
                bool get() { return dragging; }
            }

        protected:
            virtual void OnMouseDown(MouseEventArgs^ e) override
            {
                if (e->Button == System::Windows::Forms::MouseButtons::Left) {
                    // Pressing the thumb keeps the grab point; pressing the track jumps there.
                    float thumbX = ThumbX(value);
                    grabOffset = 0.0f;
                    if (SnapToEnds && Math::Abs(e->X - thumbX) <= ThumbRadius())
                        grabOffset = e->X - thumbX;
                    dragging = true;
                    SetFromX(e->X);
                }
                Control::OnMouseDown(e);
            }

            virtual void OnMouseMove(MouseEventArgs^ e) override
            {
                if (dragging) SetFromX(e->X);
                Control::OnMouseMove(e);
            }

            virtual void OnMouseUp(MouseEventArgs^ e) override
            {
                if (dragging && e->Button == System::Windows::Forms::MouseButtons::Left) {
                    SetFromX(e->X);
                    dragging = false;
                    ValueCommitted(this, EventArgs::Empty);
                }
                Control::OnMouseUp(e);
            }

            // Capture lost mid-drag (e.g. Alt+Tab): cancel instead of seeking somewhere unexpected.
            virtual void OnMouseCaptureChanged(EventArgs^ e) override
            {
                dragging = false;
                Control::OnMouseCaptureChanged(e);
            }

            // The thumb travels between r and Width - r, so 0 and 1 sit on the track ends.
            virtual void OnPaint(PaintEventArgs^ e) override
            {
                Graphics^ g = e->Graphics;
                g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;
                float r = ThumbRadius();
                float h = (dragging ? 5.0f : 4.0f) * this->DeviceDpi / 96.0f;  // track thickness
                float y = this->Height / 2.0f;
                float x = ThumbX(value);

                SolidBrush^ track = gcnew SolidBrush(Color::FromArgb(90, 90, 90));
                SolidBrush^ fill = gcnew SolidBrush(FillColor);
                g->FillRectangle(track, 0.0f, y - h / 2, (float)this->Width, h);
                g->FillRectangle(fill, 0.0f, y - h / 2, x, h);
                g->FillEllipse(fill, x - r, y - r, 2 * r, 2 * r);
                delete track;
                delete fill;
            }

        private:
            double value;
            bool dragging;
            float grabOffset;  // pointer x minus thumb centre at the start of a drag

            // Shared by drawing and hit-testing, so they always agree.
            float ThumbRadius() { return 6.0f * this->DeviceDpi / 96.0f; }

            float TravelWidth() { return Math::Max(1.0f, this->Width - 2.0f * ThumbRadius()); }

            float ThumbX(double v) { return ThumbRadius() + TravelWidth() * (float)v; }

            // Inverse of ThumbX. Positions past either end clamp to 0 or 1.
            void SetFromX(int px)
            {
                double v = (px - grabOffset - ThumbRadius()) / TravelWidth();
                if (SnapToEnds) {
                    double snap = ThumbRadius() / TravelWidth();
                    if (v <= snap) v = 0.0;
                    else if (v >= 1.0 - snap) v = 1.0;
                }
                value = Math::Max(0.0, Math::Min(1.0, v));
                Invalidate();
                ValueChanged(this, EventArgs::Empty);
            }
        };

    private:
        // A button that can be clicked but never takes keyboard focus.
        ref class NoFocusButton : public System::Windows::Forms::Button
        {
        public:
            NoFocusButton() { SetStyle(ControlStyles::Selectable, false); }
        };

    public:
        SeekBar^ seekBar;
        SeekBar^ volumeBar;
        Label^ lblTime;
        Button^ btnPrev;
        Button^ btnBack;
        Button^ btnPlayPause;
        Button^ btnFwd;
        Button^ btnNext;
        Button^ btnRepeat;
        Button^ btnShuffle;
        Button^ btnVocals;
        Button^ btnExit;

        // Set by the player. Forwards key presses in case this window ever has focus.
        Func<Keys, bool>^ ShortcutHandler;

        // True while a mouse button is held on one of the bar's controls.
        property bool IsPressed
        {
            bool get() { return pressed; }
        }

        // A pressed control keeps mouse capture, so the release is seen even off the bar.
        void TrackPress(Control^ c)
        {
            c->MouseDown += gcnew MouseEventHandler(this, &FullScreenOverlay::OnPressDown);
            c->MouseUp += gcnew MouseEventHandler(this, &FullScreenOverlay::OnPressUp);
            c->MouseCaptureChanged += gcnew EventHandler(this, &FullScreenOverlay::OnPressLost);
        }

        // Green text means "on", matching the normal window.
        void SetToggle(Button^ b, String^ text, bool on)
        {
            b->Text = text;
            b->ForeColor = on ? Color::FromArgb(120, 220, 120) : Color::White;
        }

        FullScreenOverlay()
        {
            this->SuspendLayout();

            // 100%-scale layout for a 1040-wide bar; anchors spread it to the screen width.
            AnchorStyles leftTop = static_cast<AnchorStyles>(AnchorStyles::Top | AnchorStyles::Left);
            AnchorStyles rightTop = static_cast<AnchorStyles>(AnchorStyles::Top | AnchorStyles::Right);

            seekBar = gcnew SeekBar();
            seekBar->Location = Point(12, 4);
            seekBar->Size = System::Drawing::Size(1016, 22);
            seekBar->Anchor = static_cast<AnchorStyles>(AnchorStyles::Top | AnchorStyles::Left | AnchorStyles::Right);

            btnPrev      = MakeButton(L"|< Prev", 12, 60, leftTop);
            btnBack      = MakeButton(L"-10s", 76, 50, leftTop);
            btnPlayPause = MakeButton(L"Pause", 130, 64, leftTop);
            btnFwd       = MakeButton(L"+10s", 198, 50, leftTop);
            btnNext      = MakeButton(L"Next >|", 252, 60, leftTop);

            lblTime = gcnew Label();
            lblTime->Location = Point(322, 30);
            lblTime->Size = System::Drawing::Size(124, 30);
            lblTime->Anchor = leftTop;
            lblTime->TextAlign = ContentAlignment::MiddleLeft;
            lblTime->ForeColor = Color::Gainsboro;
            lblTime->Text = L"0:00 / 0:00";

            Label^ lblVol = gcnew Label();
            lblVol->Location = Point(450, 30);
            lblVol->Size = System::Drawing::Size(30, 30);
            lblVol->Anchor = leftTop;
            lblVol->TextAlign = ContentAlignment::MiddleLeft;
            lblVol->ForeColor = Color::Gainsboro;
            lblVol->Text = L"Vol";

            volumeBar = gcnew SeekBar();
            // Gap after the "Vol" label so the thumb's left end isn't crowded.
            volumeBar->Location = Point(496, 34);
            volumeBar->Size = System::Drawing::Size(100, 22);
            volumeBar->Anchor = leftTop;
            volumeBar->FillColor = Color::WhiteSmoke;
            volumeBar->SnapToEnds = true;

            btnExit    = MakeButton(L"Exit Full Screen", 908, 120, rightTop);
            btnVocals  = MakeButton(L"Vocals ON", 814, 90, rightTop);
            btnShuffle = MakeButton(L"Shuffle: Off", 714, 96, rightTop);
            btnRepeat  = MakeButton(L"Repeat: Off", 614, 96, rightTop);

            // Sizes are for 100% scaling; WinForms scales them for the current DPI.
            this->AutoScaleDimensions = SizeF(96, 96);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Dpi;
            this->Font = gcnew System::Drawing::Font(L"Segoe UI", 9);
            this->ClientSize = System::Drawing::Size(1040, 66);
            this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
            this->ShowInTaskbar = false;
            this->StartPosition = FormStartPosition::Manual;
            this->BackColor = Color::FromArgb(24, 24, 24);
            this->Opacity = 0.85;  // set before the window exists, so no handle recreation

            array<Control^>^ all = { seekBar, btnPrev, btnBack, btnPlayPause, btnFwd, btnNext,
                                     lblTime, lblVol, volumeBar, btnRepeat, btnShuffle, btnVocals, btnExit };
            this->Controls->AddRange(all);
            TrackPress(seekBar);
            TrackPress(volumeBar);
            for each (Control^ c in all) {
                if (dynamic_cast<Button^>(c) != nullptr) TrackPress(c);
            }

            this->ResumeLayout(false);
            this->PerformLayout();
        }

    private:
        bool pressed;

        void OnPressDown(Object^ sender, MouseEventArgs^ e) { pressed = true; }
        void OnPressUp(Object^ sender, MouseEventArgs^ e) { pressed = false; }
        // Capture lost without a normal release (e.g. Alt+Tab): don't stay stuck.
        void OnPressLost(Object^ sender, EventArgs^ e) { pressed = false; }

        Button^ MakeButton(String^ text, int x, int width, AnchorStyles anchor)
        {
            Button^ b = gcnew NoFocusButton();
            b->Text = text;
            b->Location = Point(x, 30);
            b->Size = System::Drawing::Size(width, 30);
            b->Anchor = anchor;
            b->FlatStyle = FlatStyle::Flat;
            b->FlatAppearance->BorderSize = 0;
            b->FlatAppearance->MouseOverBackColor = Color::FromArgb(60, 60, 60);
            b->FlatAppearance->MouseDownBackColor = Color::FromArgb(80, 80, 80);
            b->BackColor = Color::FromArgb(24, 24, 24);
            b->ForeColor = Color::White;
            b->TabStop = false;
            return b;
        }

    protected:
        property bool ShowWithoutActivation
        {
            virtual bool get() override { return true; }
        }

        property System::Windows::Forms::CreateParams^ CreateParams
        {
            virtual System::Windows::Forms::CreateParams^ get() override
            {
                System::Windows::Forms::CreateParams^ cp = System::Windows::Forms::Form::CreateParams;
                cp->ExStyle |= 0x08000000;  // WS_EX_NOACTIVATE: clicks don't activate it
                cp->ExStyle |= 0x00000080;  // WS_EX_TOOLWINDOW: no taskbar or Alt+Tab entry
                return cp;
            }
        }

        virtual bool ProcessCmdKey(System::Windows::Forms::Message% msg, Keys keyData) override
        {
            if (ShortcutHandler != nullptr && ShortcutHandler(keyData)) return true;
            return System::Windows::Forms::Form::ProcessCmdKey(msg, keyData);
        }
    };
}
