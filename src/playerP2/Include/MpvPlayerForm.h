#pragma once

#include <mpv/client.h>
#include <msclr/marshal_cppstd.h> 
#include <string> 
#include "MediaItem.h"
#include "FullScreenOverlay.h"

namespace MyMediaPlayer {

    using namespace System;
    using namespace System::Windows::Forms;
    using namespace System::IO;

    // The Windows Forms designer needs this form to be the first class in the namespace.
    public ref class MpvPlayerForm : public System::Windows::Forms::Form
    {
    private:
        System::Windows::Forms::Panel^ panel1;
        System::Windows::Forms::ListBox^ lstPlaylist;

        System::Windows::Forms::Button^ btnAddFiles;
        System::Windows::Forms::Button^ btnPause;
        System::Windows::Forms::Button^ btnStop;
        System::Windows::Forms::Button^ btnForward;
        System::Windows::Forms::Button^ btnBackward;
        System::Windows::Forms::Button^ btnTrack;
        System::Windows::Forms::Button^ btnVocal;
        System::Windows::Forms::Button^ btnPrevious;
        System::Windows::Forms::Button^ btnNext;
        System::Windows::Forms::Button^ btnRepeat;
        System::Windows::Forms::Button^ btnShuffle;
        System::Windows::Forms::Button^ btnFullScreen;

        System::Windows::Forms::TrackBar^ tbPosition;
        System::Windows::Forms::TrackBar^ tbVolume;

        System::Windows::Forms::Timer^ tmrUpdate;

        mpv_handle* mpv;
    private: System::ComponentModel::IContainer^ components;
           bool isUserSeeking;
           bool isVocalReduced; // true while the @vocal audio filter is in mpv's chain
           int currentIndex;    // item loaded in mpv (-1 = none), separate from the highlighted row
           long long activeEntryId; // mpv entry id of the file we loaded (-1 = none); other ids are stale
           bool adoptNextStart;     // fallback: take the id from the next start-file event
           int consecutiveErrors;   // broken files skipped in a row (stops endless skipping)
           enum class RepeatMode { Off, All, One };
           RepeatMode repeatMode;
           // Shuffle order for this cycle. Items before shufflePos have already played.
           bool isShuffle;
           System::Collections::Generic::List<int>^ shuffleOrder;
           int shufflePos;          // position of the current item in shuffleOrder (-1 = none)
           System::Random^ rng;
           // Saved window state, restored when leaving full screen.
           bool isFullScreen;
           System::IntPtr videoHandle;  // panel1 handle given to mpv as 'wid'
           System::Windows::Forms::FormBorderStyle savedBorderStyle;
           System::Windows::Forms::FormWindowState savedWindowState;
           System::Drawing::Rectangle savedBounds;       // normal (not maximised) window bounds
           System::Drawing::Rectangle savedPanelBounds;
           System::Windows::Forms::AnchorStyles savedPanelAnchor;
           // Floating control bar; exists only in full screen.
           FullScreenOverlay^ fsOverlay;
           System::Windows::Forms::Timer^ tmrFsMouse;  // watches the mouse in full screen
           System::Drawing::Point activityPos;         // pointer position at the last real movement
           bool fsIdle;                                // bar and pointer are hidden
           bool pointerHidden;                         // we called Cursor::Hide() and owe a Show()
           unsigned int lastActivityTick;              // Environment::TickCount of last activity

    public:
        MpvPlayerForm(void)
        {
            isUserSeeking = false;
            isVocalReduced = false;
            currentIndex = -1;
            activeEntryId = -1;
            adoptNextStart = false;
            consecutiveErrors = 0;
            repeatMode = RepeatMode::Off;
            isShuffle = false;
            shuffleOrder = gcnew System::Collections::Generic::List<int>();
            shufflePos = -1;
            rng = gcnew System::Random();
            isFullScreen = false;
            InitializeComponent();
            // Stop the window shrinking until controls overlap.
            // Set after InitializeComponent so it uses the DPI-scaled size.
            this->MinimumSize = this->Size;
            UpdateVocalButton();
            UpdateNavButtons();
            UpdateRepeatButton();
            UpdateShuffleButton();
            this->Activated += gcnew System::EventHandler(this, &MpvPlayerForm::MpvPlayerForm_Activated);
            this->Deactivate += gcnew System::EventHandler(this, &MpvPlayerForm::MpvPlayerForm_Deactivate);
            this->FormClosing += gcnew FormClosingEventHandler(this, &MpvPlayerForm::MpvPlayerForm_FormClosing);
            tmrFsMouse = gcnew System::Windows::Forms::Timer(this->components);
            tmrFsMouse->Interval = 150;
            tmrFsMouse->Tick += gcnew System::EventHandler(this, &MpvPlayerForm::tmrFsMouse_Tick);

            // Don't start libmpv inside the Visual Studio designer.
            if (System::ComponentModel::LicenseManager::UsageMode !=
                System::ComponentModel::LicenseUsageMode::Designtime)
            {
                InitializeMpv();
            }
        }

    protected:
        // Runs before the focused control sees the key, so shortcuts work everywhere.
        virtual bool ProcessCmdKey(System::Windows::Forms::Message% msg, System::Windows::Forms::Keys keyData) override
        {
            if (HandleShortcut(keyData)) return true;
            return System::Windows::Forms::Form::ProcessCmdKey(msg, keyData);
        }

        // keyData includes modifiers, so Ctrl+F, Shift+Space etc. are ignored.
        bool HandleShortcut(System::Windows::Forms::Keys keyData)
        {
            typedef System::Windows::Forms::Keys K;

            if (keyData == K::Escape) {
                if (!isFullScreen) return false;
                ExitFullScreen();
                return true;
            }

            // Leave keys to controls that need them: typing in a text box,
            // and the arrow keys on a focused slider.
            Control^ focused = this->ActiveControl;
            if (dynamic_cast<TextBoxBase^>(focused) != nullptr ||
                dynamic_cast<ComboBox^>(focused) != nullptr ||
                dynamic_cast<NumericUpDown^>(focused) != nullptr)
                return false;
            if ((keyData == K::Left || keyData == K::Right) && dynamic_cast<TrackBar^>(focused) != nullptr)
                return false;

            if (keyData == K::F) {
                if (isFullScreen) ExitFullScreen();
                else EnterFullScreen();
            }
            else if (keyData == K::Space) btnPause_Click(this, System::EventArgs::Empty);
            else if (keyData == K::Left)  btnBackward_Click(this, System::EventArgs::Empty);
            else if (keyData == K::Right) btnForward_Click(this, System::EventArgs::Empty);
            else if (keyData == K::N)     btnNext_Click(this, System::EventArgs::Empty);
            else if (keyData == K::P)     btnPrevious_Click(this, System::EventArgs::Empty);
            else return false;
            return true;
        }

        ~MpvPlayerForm()
        {
            if (mpv) {
                mpv_terminate_destroy(mpv);
                mpv = nullptr;
            }
        }

    private:
        void InitializeComponent(void)
        {
            this->components = (gcnew System::ComponentModel::Container());
            this->panel1 = (gcnew System::Windows::Forms::Panel());
            this->lstPlaylist = (gcnew System::Windows::Forms::ListBox());
            this->btnAddFiles = (gcnew System::Windows::Forms::Button());
            this->btnPause = (gcnew System::Windows::Forms::Button());
            this->btnStop = (gcnew System::Windows::Forms::Button());
            this->btnForward = (gcnew System::Windows::Forms::Button());
            this->btnBackward = (gcnew System::Windows::Forms::Button());
            this->btnTrack = (gcnew System::Windows::Forms::Button());
            this->btnVocal = (gcnew System::Windows::Forms::Button());
            this->btnPrevious = (gcnew System::Windows::Forms::Button());
            this->btnNext = (gcnew System::Windows::Forms::Button());
            this->btnRepeat = (gcnew System::Windows::Forms::Button());
            this->btnShuffle = (gcnew System::Windows::Forms::Button());
            this->btnFullScreen = (gcnew System::Windows::Forms::Button());
            this->tbPosition = (gcnew System::Windows::Forms::TrackBar());
            this->tbVolume = (gcnew System::Windows::Forms::TrackBar());
            this->tmrUpdate = (gcnew System::Windows::Forms::Timer(this->components));
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->tbPosition))->BeginInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->tbVolume))->BeginInit();
            this->SuspendLayout();
            this->panel1->BackColor = System::Drawing::Color::Black;
            this->panel1->Location = System::Drawing::Point(12, 12);
            this->panel1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) | System::Windows::Forms::AnchorStyles::Left) | System::Windows::Forms::AnchorStyles::Right));
            this->panel1->Name = L"panel1";
            this->panel1->Size = System::Drawing::Size(600, 360);
            this->panel1->TabIndex = 0;
            this->lstPlaylist->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
                static_cast<System::Byte>(0)));
            this->lstPlaylist->ItemHeight = 25;
            this->lstPlaylist->Location = System::Drawing::Point(620, 12);
            this->lstPlaylist->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) | System::Windows::Forms::AnchorStyles::Right));
            this->lstPlaylist->Name = L"lstPlaylist";
            this->lstPlaylist->Size = System::Drawing::Size(250, 491);
            this->lstPlaylist->TabIndex = 1;
            this->lstPlaylist->DoubleClick += gcnew System::EventHandler(this, &MpvPlayerForm::lstPlaylist_DoubleClick);
            this->btnAddFiles->Location = System::Drawing::Point(12, 430);
            this->btnAddFiles->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->btnAddFiles->Name = L"btnAddFiles";
            this->btnAddFiles->Size = System::Drawing::Size(70, 30);
            this->btnAddFiles->TabIndex = 3;
            this->btnAddFiles->Text = L"Add";
            this->btnAddFiles->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnAddFiles_Click);
            this->btnPause->Location = System::Drawing::Point(145, 430);
            this->btnPause->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->btnPause->Name = L"btnPause";
            this->btnPause->Size = System::Drawing::Size(60, 30);
            this->btnPause->TabIndex = 5;
            this->btnPause->Text = L"Pause";
            this->btnPause->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnPause_Click);
            this->btnStop->Location = System::Drawing::Point(265, 430);
            this->btnStop->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->btnStop->Name = L"btnStop";
            this->btnStop->Size = System::Drawing::Size(50, 30);
            this->btnStop->TabIndex = 7;
            this->btnStop->Text = L"Stop";
            this->btnStop->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnStop_Click);
            this->btnForward->Location = System::Drawing::Point(210, 430);
            this->btnForward->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->btnForward->Name = L"btnForward";
            this->btnForward->Size = System::Drawing::Size(50, 30);
            this->btnForward->TabIndex = 6;
            this->btnForward->Text = L"10s >>";
            this->btnForward->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnForward_Click);
            this->btnBackward->Location = System::Drawing::Point(90, 430);
            this->btnBackward->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->btnBackward->Name = L"btnBackward";
            this->btnBackward->Size = System::Drawing::Size(50, 30);
            this->btnBackward->TabIndex = 4;
            this->btnBackward->Text = L"<< 10s";
            this->btnBackward->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnBackward_Click);
            this->btnTrack->Location = System::Drawing::Point(320, 430);
            this->btnTrack->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->btnTrack->Name = L"btnTrack";
            this->btnTrack->Size = System::Drawing::Size(70, 30);
            this->btnTrack->TabIndex = 8;
            this->btnTrack->Text = L"Track";
            this->btnTrack->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnTrack_Click);
            this->btnVocal->Location = System::Drawing::Point(395, 430);
            this->btnVocal->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->btnVocal->Name = L"btnVocal";
            this->btnVocal->Size = System::Drawing::Size(80, 30);
            this->btnVocal->TabIndex = 9;
            this->btnVocal->Text = L"Vocal Off";
            this->btnVocal->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnVocal_Click);
            this->btnPrevious->Location = System::Drawing::Point(12, 470);
            this->btnPrevious->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->btnPrevious->Name = L"btnPrevious";
            this->btnPrevious->Size = System::Drawing::Size(70, 30);
            this->btnPrevious->TabIndex = 11;
            this->btnPrevious->Text = L"|< Prev";
            this->btnPrevious->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnPrevious_Click);
            this->btnNext->Location = System::Drawing::Point(90, 470);
            this->btnNext->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->btnNext->Name = L"btnNext";
            this->btnNext->Size = System::Drawing::Size(70, 30);
            this->btnNext->TabIndex = 12;
            this->btnNext->Text = L"Next >|";
            this->btnNext->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnNext_Click);
            this->btnRepeat->Location = System::Drawing::Point(168, 470);
            this->btnRepeat->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->btnRepeat->Name = L"btnRepeat";
            this->btnRepeat->Size = System::Drawing::Size(90, 30);
            this->btnRepeat->TabIndex = 13;
            this->btnRepeat->Text = L"Repeat: Off";
            this->btnRepeat->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnRepeat_Click);
            this->btnShuffle->Location = System::Drawing::Point(266, 470);
            this->btnShuffle->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->btnShuffle->Name = L"btnShuffle";
            this->btnShuffle->Size = System::Drawing::Size(90, 30);
            this->btnShuffle->TabIndex = 14;
            this->btnShuffle->Text = L"Shuffle: Off";
            this->btnShuffle->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnShuffle_Click);
            this->btnFullScreen->Location = System::Drawing::Point(364, 470);
            this->btnFullScreen->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->btnFullScreen->Name = L"btnFullScreen";
            this->btnFullScreen->Size = System::Drawing::Size(90, 30);
            this->btnFullScreen->TabIndex = 15;
            this->btnFullScreen->Text = L"Full Screen";
            this->btnFullScreen->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnFullScreen_Click);
            this->tbPosition->Location = System::Drawing::Point(12, 380);
            this->tbPosition->Maximum = 1000;
            this->tbPosition->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left) | System::Windows::Forms::AnchorStyles::Right));
            this->tbPosition->Name = L"tbPosition";
            this->tbPosition->Size = System::Drawing::Size(600, 45);
            this->tbPosition->TabIndex = 2;
            this->tbPosition->TickStyle = System::Windows::Forms::TickStyle::None;
            this->tbPosition->Scroll += gcnew System::EventHandler(this, &MpvPlayerForm::tbPosition_Scroll);
            this->tbPosition->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MpvPlayerForm::tbPosition_MouseDown);
            this->tbPosition->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MpvPlayerForm::tbPosition_MouseUp);
            this->tbVolume->Location = System::Drawing::Point(485, 430);
            this->tbVolume->Maximum = 100;
            this->tbVolume->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
            this->tbVolume->Name = L"tbVolume";
            this->tbVolume->Size = System::Drawing::Size(125, 45);
            this->tbVolume->TabIndex = 10;
            this->tbVolume->TickStyle = System::Windows::Forms::TickStyle::None;
            this->tbVolume->Value = 100;
            this->tbVolume->Scroll += gcnew System::EventHandler(this, &MpvPlayerForm::tbVolume_Scroll);
            this->tmrUpdate->Interval = 500;
            this->tmrUpdate->Tick += gcnew System::EventHandler(this, &MpvPlayerForm::tmrUpdate_Tick);
            this->AutoScaleDimensions = System::Drawing::SizeF(96, 96);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Dpi;
            this->ClientSize = System::Drawing::Size(885, 512);
            this->Controls->Add(this->panel1);
            this->Controls->Add(this->lstPlaylist);
            this->Controls->Add(this->tbPosition);
            this->Controls->Add(this->btnAddFiles);
            this->Controls->Add(this->btnBackward);
            this->Controls->Add(this->btnPause);
            this->Controls->Add(this->btnForward);
            this->Controls->Add(this->btnStop);
            this->Controls->Add(this->btnTrack);
            this->Controls->Add(this->btnVocal);
            this->Controls->Add(this->tbVolume);
            this->Controls->Add(this->btnPrevious);
            this->Controls->Add(this->btnNext);
            this->Controls->Add(this->btnRepeat);
            this->Controls->Add(this->btnShuffle);
            this->Controls->Add(this->btnFullScreen);
            this->Name = L"MpvPlayerForm";
            this->Text = L"C++/CLI libmpv Full Karaoke Player";
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->tbPosition))->EndInit();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->tbVolume))->EndInit();
            this->ResumeLayout(false);
            this->PerformLayout();

        }

        void InitializeMpv()
        {
            mpv = mpv_create();
            if (!mpv) {
                MessageBox::Show("Failed to create mpv instance.");
                return;
            }

            videoHandle = this->panel1->Handle;
            int64_t wid = videoHandle.ToInt64();
            mpv_set_property(mpv, "wid", MPV_FORMAT_INT64, &wid);
            // Normal window only; in full screen the player hides the pointer (SetPointerHidden).
            mpv_set_option_string(mpv, "cursor-autohide", "3000");

            if (mpv_initialize(mpv) < 0) {
                MessageBox::Show("Failed to initialize mpv.");
                return;
            }

            this->tmrUpdate->Start();
        }

        void btnAddFiles_Click(System::Object^ sender, System::EventArgs^ e)
        {
            OpenFileDialog^ openFileDialog = gcnew OpenFileDialog();
            openFileDialog->Filter = L"Media Files|*.mp4;*.mp3;*.mkv;*.avi;*.wav|All Files|*.*";
            openFileDialog->Multiselect = true;

            if (openFileDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
            {
                int firstNew = this->lstPlaylist->Items->Count;
                for each (String ^ file in openFileDialog->FileNames)
                {
                    this->lstPlaylist->Items->Add(gcnew MediaItem(file));
                }
                AddToShuffleOrder(firstNew);

                if (this->lstPlaylist->Items->Count > 0 && this->lstPlaylist->SelectedIndex == -1)
                {
                    // Nothing played yet: start at the top, or at the first shuffled item.
                    int first = 0;
                    if (isShuffle && shufflePos + 1 < shuffleOrder->Count)
                        first = shuffleOrder[shufflePos + 1];
                    PlayIndex(first);
                }
                UpdateNavButtons(); // new items may make Next available
            }
        }

        void lstPlaylist_DoubleClick(System::Object^ sender, System::EventArgs^ e)
        {
            if (this->lstPlaylist->SelectedIndex != -1)
            {
                PlayIndex(this->lstPlaylist->SelectedIndex);
            }
        }

        // Single entry point for playing a playlist item.
        void PlayIndex(int index)
        {
            if (index < 0 || index >= this->lstPlaylist->Items->Count) return;

            MediaItem^ item = safe_cast<MediaItem^>(this->lstPlaylist->Items[index]);
            currentIndex = index;
            consecutiveErrors = 0; // AdvanceAfterEnd restores it for auto-skips
            SyncShufflePosition(index);
            this->lstPlaylist->SelectedIndex = index;
            PlayFile(item->FullPath);
            UpdateNavButtons();
        }

        void btnPrevious_Click(System::Object^ sender, System::EventArgs^ e)
        {
            if (isShuffle) {
                if (shufflePos > 0) PlayIndex(shuffleOrder[shufflePos - 1]);
                return;
            }
            // First item (or nothing played yet): do nothing.
            if (currentIndex > 0) {
                PlayIndex(currentIndex - 1);
            }
        }

        void btnNext_Click(System::Object^ sender, System::EventArgs^ e)
        {
            if (isShuffle) {
                if (shufflePos >= 0 && shufflePos < shuffleOrder->Count - 1)
                    PlayIndex(shuffleOrder[shufflePos + 1]);
                return;
            }
            // Last item (or nothing played yet): do nothing.
            if (currentIndex >= 0 && currentIndex < this->lstPlaylist->Items->Count - 1) {
                PlayIndex(currentIndex + 1);
            }
        }

        // Grey out Previous/Next when they would do nothing.
        void UpdateNavButtons()
        {
            if (isShuffle) {
                this->btnPrevious->Enabled = (shufflePos > 0);
                this->btnNext->Enabled = (shufflePos >= 0 && shufflePos < shuffleOrder->Count - 1);
            }
            else {
                int count = this->lstPlaylist->Items->Count;
                this->btnPrevious->Enabled = (currentIndex > 0 && currentIndex < count);
                this->btnNext->Enabled = (currentIndex >= 0 && currentIndex < count - 1);
            }
            if (fsOverlay != nullptr) {
                fsOverlay->btnPrev->Enabled = this->btnPrevious->Enabled;
                fsOverlay->btnNext->Enabled = this->btnNext->Enabled;
            }
        }

        void PlayFile(String^ filePath)
        {
            if (!mpv) return;

            array<Byte>^ utf8Bytes = System::Text::Encoding::UTF8->GetBytes(filePath);
            cli::pin_ptr<Byte> pinnedBytes = &utf8Bytes[0];
            std::string nativeUtf8Path(reinterpret_cast<char*>(pinnedBytes), utf8Bytes->Length);

            const char* cmd[] = { "loadfile", nativeUtf8Path.c_str(), NULL };

            // loadfile returns the new entry's id; events for older files are then ignored.
            activeEntryId = -1;
            adoptNextStart = false;
            mpv_node result;
            if (mpv_command_ret(mpv, cmd, &result) >= 0) {
                if (result.format == MPV_FORMAT_NODE_MAP) {
                    for (int i = 0; i < result.u.list->num; i++) {
                        if (std::string(result.u.list->keys[i]) == "playlist_entry_id" &&
                            result.u.list->values[i].format == MPV_FORMAT_INT64) {
                            activeEntryId = result.u.list->values[i].u.int64;
                        }
                    }
                }
                mpv_free_node_contents(&result);
                if (activeEntryId < 0) adoptNextStart = true; // id not returned: use start-file
            }

            int pause_val = 0;
            mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &pause_val);

            double vol = this->tbVolume->Value;
            mpv_set_property(mpv, "volume", MPV_FORMAT_DOUBLE, &vol);
        }

        void btnPause_Click(System::Object^ sender, System::EventArgs^ e)
        {
            if (!mpv) return;
            const char* cmd[] = { "cycle", "pause", NULL };
            mpv_command(mpv, cmd);
        }

        void btnStop_Click(System::Object^ sender, System::EventArgs^ e)
        {
            if (!mpv) return;
            activeEntryId = -1;      // never autoplay after Stop
            adoptNextStart = false;
            const char* cmd[] = { "stop", NULL };
            mpv_command(mpv, cmd);
            this->tbPosition->Value = 0;
        }

        void btnForward_Click(System::Object^ sender, System::EventArgs^ e)
        {
            if (!mpv) return;
            const char* cmd[] = { "seek", "10", NULL };
            mpv_command(mpv, cmd);
        }

        void btnBackward_Click(System::Object^ sender, System::EventArgs^ e)
        {
            if (!mpv) return;
            const char* cmd[] = { "seek", "-10", NULL };
            mpv_command(mpv, cmd);
        }

        void tbVolume_Scroll(System::Object^ sender, System::EventArgs^ e)
        {
            if (!mpv) return;
            double vol = this->tbVolume->Value;
            mpv_set_property(mpv, "volume", MPV_FORMAT_DOUBLE, &vol);
        }

        void tbPosition_MouseDown(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
        {
            isUserSeeking = true;
        }

        void tbPosition_MouseUp(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
        {
            if (!mpv) return;
            SeekToPercent(this->tbPosition->Value / 10.0);
            isUserSeeking = false;
        }

        void tbPosition_Scroll(System::Object^ sender, System::EventArgs^ e)
        {
            if (!isUserSeeking && mpv) {
                SeekToPercent(this->tbPosition->Value / 10.0);
            }
        }

        void SeekToPercent(double percent)
        {
            if (!mpv) return;
            mpv_set_property(mpv, "percent-pos", MPV_FORMAT_DOUBLE, &percent);
        }

        void tmrUpdate_Tick(System::Object^ sender, System::EventArgs^ e)
        {
            if (!mpv) return;
            ProcessMpvEvents();
            if (isUserSeeking) return;

            double percent = 0;
            mpv_get_property(mpv, "percent-pos", MPV_FORMAT_DOUBLE, &percent);

            if (percent >= 0 && percent <= 100) {
                this->tbPosition->Value = (int)(percent * 10);
            }
        }

        // Drains libmpv events on the UI thread. Only an end-file for activeEntryId counts:
        // eof plays the next item, error skips a broken file, stop/quit are ignored.
        void ProcessMpvEvents()
        {
            bool endedNaturally = false;
            bool endedWithError = false;

            for (int n = 0; n < 200; n++) {           // cap per tick keeps the UI responsive
                mpv_event* ev = mpv_wait_event(mpv, 0); // 0 = never block
                if (ev->event_id == MPV_EVENT_NONE || ev->event_id == MPV_EVENT_SHUTDOWN) break;

                if (ev->event_id == MPV_EVENT_START_FILE && adoptNextStart) {
                    activeEntryId = static_cast<mpv_event_start_file*>(ev->data)->playlist_entry_id;
                    adoptNextStart = false;
                }
                else if (ev->event_id == MPV_EVENT_END_FILE) {
                    mpv_event_end_file* ef = static_cast<mpv_event_end_file*>(ev->data);
                    if (activeEntryId < 0 || ef->playlist_entry_id != activeEntryId)
                        continue;                      // ignore old events to prevent double skipping

                    activeEntryId = -1;                // this file is finished, whatever the reason
                    if (ef->reason == MPV_END_FILE_REASON_EOF) endedNaturally = true;
                    else if (ef->reason == MPV_END_FILE_REASON_ERROR) endedWithError = true;
                }
            }

            if (endedNaturally) {
                consecutiveErrors = 0;
                AdvanceAfterEnd(true);
            }
            else if (endedWithError) {
                consecutiveErrors++;
                if (consecutiveErrors < this->lstPlaylist->Items->Count) // gives up after one full pass
                    AdvanceAfterEnd(false);
            }
        }

        // Repeat One never replays a broken file, otherwise it would fail forever.
        void AdvanceAfterEnd(bool finishedNaturally)
        {
            int count = this->lstPlaylist->Items->Count;
            if (currentIndex < 0 || currentIndex >= count) return;

            int next;
            if (finishedNaturally && repeatMode == RepeatMode::One) {
                next = currentIndex;
            }
            else if (isShuffle) {
                if (shufflePos + 1 < shuffleOrder->Count) {
                    next = shuffleOrder[shufflePos + 1];
                }
                else {
                    if (repeatMode != RepeatMode::All) return; // cycle finished: stop
                    StartNewShuffleCycle();
                    next = shuffleOrder[0];
                }
            }
            else {
                next = currentIndex + 1;
                if (next >= count) {
                    if (repeatMode != RepeatMode::All) return; // end of list: stop
                    next = 0;
                }
            }

            int errors = consecutiveErrors;  // PlayIndex resets it; keep the count for auto-skips
            PlayIndex(next);
            consecutiveErrors = errors;
        }

        // Off -> All -> One. Only affects what happens when a file ends.
        void btnRepeat_Click(System::Object^ sender, System::EventArgs^ e)
        {
            if (repeatMode == RepeatMode::Off)      repeatMode = RepeatMode::All;
            else if (repeatMode == RepeatMode::All) repeatMode = RepeatMode::One;
            else                                    repeatMode = RepeatMode::Off;
            UpdateRepeatButton();
        }

        // Same style as the Vocals button: green while a repeat mode is on.
        void UpdateRepeatButton()
        {
            if (repeatMode == RepeatMode::Off) {
                this->btnRepeat->Text = L"Repeat: Off";
                this->btnRepeat->ResetBackColor();
                this->btnRepeat->UseVisualStyleBackColor = true;
            }
            else {
                this->btnRepeat->Text = (repeatMode == RepeatMode::All) ? L"Repeat: All" : L"Repeat: One";
                this->btnRepeat->UseVisualStyleBackColor = false;
                this->btnRepeat->BackColor = System::Drawing::Color::LightGreen;
            }
            if (fsOverlay != nullptr)
                fsOverlay->SetToggle(fsOverlay->btnRepeat, this->btnRepeat->Text, repeatMode != RepeatMode::Off);
        }

        // Turning shuffle on or off never reloads the current video.
        void btnShuffle_Click(System::Object^ sender, System::EventArgs^ e)
        {
            isShuffle = !isShuffle;
            if (isShuffle) {
                BuildShuffleOrder(currentIndex); // current video counts as already played
            }
            else {
                shuffleOrder->Clear();
                shufflePos = -1;
            }
            UpdateShuffleButton();
            UpdateNavButtons();
        }

        // keepFirst (if valid) goes first and becomes the current position.
        void BuildShuffleOrder(int keepFirst)
        {
            int count = this->lstPlaylist->Items->Count;
            bool keep = (keepFirst >= 0 && keepFirst < count);

            shuffleOrder->Clear();
            for (int i = 0; i < count; i++) {
                if (!keep || i != keepFirst) shuffleOrder->Add(i);
            }
            ShuffleFrom(0);

            if (keep) {
                shuffleOrder->Insert(0, keepFirst);
                shufflePos = 0;
            }
            else {
                shufflePos = -1;
            }
        }

        // Avoid playing the video that just finished twice in a row.
        void StartNewShuffleCycle()
        {
            BuildShuffleOrder(-1);
            int n = shuffleOrder->Count;
            if (n > 1 && shuffleOrder[0] == currentIndex) {
                int j = rng->Next(1, n);
                shuffleOrder[0] = shuffleOrder[j];
                shuffleOrder[j] = currentIndex;
            }
        }

        // Fisher-Yates shuffle from position 'from' to the end.
        void ShuffleFrom(int from)
        {
            for (int i = shuffleOrder->Count - 1; i > from; i--) {
                int j = rng->Next(from, i + 1);
                int t = shuffleOrder[i];
                shuffleOrder[i] = shuffleOrder[j];
                shuffleOrder[j] = t;
            }
        }

        // Newly added files go to random places in the not-yet-played part.
        void AddToShuffleOrder(int firstNewIndex)
        {
            if (!isShuffle) return;
            for (int i = firstNewIndex; i < this->lstPlaylist->Items->Count; i++) {
                int at = rng->Next(shufflePos + 1, shuffleOrder->Count + 1);
                shuffleOrder->Insert(at, i);
            }
        }

        // An unplayed item moves next in line so each item still plays once per cycle;
        // an already played one (e.g. Previous) just moves the position back.
        void SyncShufflePosition(int index)
        {
            if (!isShuffle) return;

            int pos = shuffleOrder->IndexOf(index);
            if (pos >= 0 && pos <= shufflePos) {
                shufflePos = pos;
                return;
            }
            if (pos > shufflePos) shuffleOrder->RemoveAt(pos);
            shuffleOrder->Insert(shufflePos + 1, index);
            shufflePos++;
        }

        void UpdateShuffleButton()
        {
            if (isShuffle) {
                this->btnShuffle->Text = L"Shuffle: On";
                this->btnShuffle->UseVisualStyleBackColor = false;
                this->btnShuffle->BackColor = System::Drawing::Color::LightGreen;
            }
            else {
                this->btnShuffle->Text = L"Shuffle: Off";
                this->btnShuffle->ResetBackColor();
                this->btnShuffle->UseVisualStyleBackColor = true;
            }
            if (fsOverlay != nullptr)
                fsOverlay->SetToggle(fsOverlay->btnShuffle, this->btnShuffle->Text, isShuffle);
        }

        void btnFullScreen_Click(System::Object^ sender, System::EventArgs^ e)
        {
            if (isFullScreen) ExitFullScreen();
            else EnterFullScreen();
        }

        // panel1 is never re-parented, so mpv keeps drawing into the same window.
        // mpv keeps the aspect ratio itself.
        void EnterFullScreen()
        {
            if (isFullScreen) return;

            savedBorderStyle = this->FormBorderStyle;
            savedWindowState = this->WindowState;
            savedBounds = (this->WindowState == FormWindowState::Normal) ? this->Bounds : this->RestoreBounds;
            savedPanelBounds = this->panel1->Bounds;
            savedPanelAnchor = this->panel1->Anchor;

            this->SuspendLayout();
            for each (Control^ c in this->Controls) {
                if (c != this->panel1) c->Visible = false;
            }
            this->panel1->Dock = DockStyle::Fill;

            // Remove the border before maximising, otherwise the taskbar stays visible.
            this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
            if (this->WindowState == FormWindowState::Maximized)
                this->WindowState = FormWindowState::Normal;
            this->WindowState = FormWindowState::Maximized;
            this->ResumeLayout(true);

            isFullScreen = true;
            KeepKeyboardFocusOnForm();
            CheckVideoHandle();
            OpenOverlay();
        }

        void ExitFullScreen()
        {
            if (!isFullScreen) return;
            isFullScreen = false;
            CloseOverlay();

            this->SuspendLayout();
            // Restore the window first, so the saved panel bounds match its client area.
            this->WindowState = FormWindowState::Normal;
            this->FormBorderStyle = savedBorderStyle;
            this->Bounds = savedBounds;
            if (savedWindowState == FormWindowState::Maximized)
                this->WindowState = FormWindowState::Maximized;

            // Setting Dock cleared the anchors, so put both back.
            this->panel1->Dock = DockStyle::None;
            this->panel1->Bounds = savedPanelBounds;
            this->panel1->Anchor = savedPanelAnchor;
            for each (Control^ c in this->Controls) {
                c->Visible = true;
            }
            this->ResumeLayout(true);

            CheckVideoHandle();
        }

        // With every control hidden, focus the form itself so ProcessCmdKey gets Esc.
        // mpv's video window never takes keyboard focus, so clicking the video is fine.
        void KeepKeyboardFocusOnForm()
        {
            this->ActiveControl = nullptr;
            this->Focus();
        }

        // Coming back to the window (e.g. after Alt+Tab) while in full screen.
        void MpvPlayerForm_Activated(System::Object^ sender, System::EventArgs^ e)
        {
            if (isFullScreen) KeepKeyboardFocusOnForm();
        }

        void OpenOverlay()
        {
            CloseOverlay();
            fsOverlay = gcnew FullScreenOverlay();
            fsOverlay->Owner = this;  // owned windows always stay above their owner
            fsOverlay->ShortcutHandler = gcnew Func<System::Windows::Forms::Keys, bool>(this, &MpvPlayerForm::HandleShortcut);
            // Overlay buttons call the same handlers as the normal buttons.
            fsOverlay->btnExit->Click += gcnew System::EventHandler(this, &MpvPlayerForm::overlayExit_Click);
            fsOverlay->btnPrev->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnPrevious_Click);
            fsOverlay->btnBack->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnBackward_Click);
            fsOverlay->btnPlayPause->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnPause_Click);
            fsOverlay->btnFwd->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnForward_Click);
            fsOverlay->btnNext->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnNext_Click);
            fsOverlay->btnRepeat->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnRepeat_Click);
            fsOverlay->btnShuffle->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnShuffle_Click);
            fsOverlay->btnVocals->Click += gcnew System::EventHandler(this, &MpvPlayerForm::btnVocal_Click);
            fsOverlay->seekBar->ValueCommitted += gcnew System::EventHandler(this, &MpvPlayerForm::overlaySeek_Committed);
            fsOverlay->volumeBar->ValueChanged += gcnew System::EventHandler(this, &MpvPlayerForm::overlayVolume_Changed);
            SyncOverlay();

            // Start the timer first, so the bar can always hide again.
            tmrFsMouse->Start();
            activityPos = System::Windows::Forms::Cursor::Position;
            fsIdle = false;
            if (mpv) mpv_set_property_string(mpv, "cursor-autohide", "no"); // player decides in full screen
            SetPointerHidden(false);
            ShowOverlay();  // shown briefly on entry
        }

        void CloseOverlay()
        {
            tmrFsMouse->Stop();
            if (fsOverlay == nullptr) return;

            // Show the pointer again and hand it back to mpv's normal 3-second autohide.
            SetPointerHidden(false);
            if (mpv) mpv_set_property_string(mpv, "cursor-autohide", "3000");

            // If the overlay somehow had focus, give it back to the player.
            bool hadFocus = fsOverlay->ContainsFocus;
            fsOverlay->Close();  // Close() also disposes a non-modal form
            fsOverlay = nullptr;
            if (hadFocus) this->Activate();
        }

        void ShowOverlay()
        {
            lastActivityTick = static_cast<unsigned int>(Environment::TickCount);
            if (fsOverlay == nullptr || fsOverlay->Visible) return;

            PositionOverlay();
            fsOverlay->Show();
            PositionOverlay();  // again, in case DPI scaling changed its size on first show
        }

        // Bar along the bottom of the monitor the player is on.
        void PositionOverlay()
        {
            System::Drawing::Rectangle screen = Screen::FromControl(this)->Bounds;
            int h = fsOverlay->Height;
            fsOverlay->Bounds = System::Drawing::Rectangle(screen.Left, screen.Bottom - h, screen.Width, h);
        }

        // The mouse moves over mpv's own window, so the form gets no mouse events
        // there. Checking the cursor position works whatever window is under it.
        void tmrFsMouse_Tick(System::Object^ sender, System::EventArgs^ e)
        {
            if (!isFullScreen || fsOverlay == nullptr) return;

            // Compare with where the last real movement happened, not the last tick,
            // so small touchpad jitter never counts as activity.
            System::Drawing::Point p = System::Windows::Forms::Cursor::Position;
            System::Drawing::Size dz = SystemInformation::DragSize;
            bool moved = Math::Abs(p.X - activityPos.X) > dz.Width ||
                         Math::Abs(p.Y - activityPos.Y) > dz.Height;
            unsigned int now = static_cast<unsigned int>(Environment::TickCount);

            if (moved && Screen::FromControl(this)->Bounds.Contains(p)) {
                activityPos = p;
                ShowOverlay();
                fsIdle = false;
                SetPointerHidden(false);
            }
            else if (fsOverlay->IsPressed) {
                lastActivityTick = now;  // clicking or dragging a control on the bar
            }
            else if (!fsIdle && now - lastActivityTick >= 3000) {
                // Hides even if the pointer rests on the bar.
                fsIdle = true;
                fsOverlay->Hide();
                // Only while the player is the active app, so other apps keep their pointer.
                if (Form::ActiveForm == this) SetPointerHidden(true);
            }

            if (fsOverlay->Visible) UpdateOverlayPlayback();
        }

        // Windows' show/hide count also covers mpv's video window (shared input queue).
        // pointerHidden keeps calls paired, so the pointer can't stay hidden.
        void SetPointerHidden(bool hidden)
        {
            if (hidden == pointerHidden) return;
            if (hidden) System::Windows::Forms::Cursor::Hide();
            else System::Windows::Forms::Cursor::Show();
            pointerHidden = hidden;
        }

        // Switching to another app: never leave our pointer hidden behind.
        void MpvPlayerForm_Deactivate(System::Object^ sender, System::EventArgs^ e)
        {
            SetPointerHidden(false);
        }

        // Copies the current player state onto a freshly created overlay.
        void SyncOverlay()
        {
            UpdateRepeatButton();
            UpdateShuffleButton();
            UpdateVocalButton();
            UpdateNavButtons();
            fsOverlay->volumeBar->Value = this->tbVolume->Value / 100.0;
            UpdateOverlayPlayback();
        }

        // Progress, time and Play/Pause label, read straight from mpv.
        void UpdateOverlayPlayback()
        {
            if (fsOverlay == nullptr || !mpv) return;

            double pos = 0, dur = 0;
            int paused = 0;
            mpv_get_property(mpv, "time-pos", MPV_FORMAT_DOUBLE, &pos);  // fail = stays 0 (nothing loaded)
            mpv_get_property(mpv, "duration", MPV_FORMAT_DOUBLE, &dur);
            mpv_get_property(mpv, "pause", MPV_FORMAT_FLAG, &paused);

            FullScreenOverlay::SeekBar^ bar = fsOverlay->seekBar;
            if (dur > 0) bar->Value = pos / dur;
            // While dragging, show the time under the thumb instead.
            double shown = (bar->IsDragging && dur > 0) ? bar->Value * dur : pos;
            fsOverlay->lblTime->Text = FormatTime(shown) + L" / " + FormatTime(dur);
            if (paused) fsOverlay->btnPlayPause->Text = L"Play";
            else fsOverlay->btnPlayPause->Text = L"Pause";
        }

        static String^ FormatTime(double seconds)
        {
            int t = (seconds > 0) ? static_cast<int>(seconds) : 0;
            int h = t / 3600, m = (t / 60) % 60, sec = t % 60;
            if (h > 0) return String::Format(L"{0}:{1:00}:{2:00}", h, m, sec);
            return String::Format(L"{0}:{1:00}", m, sec);
        }

        // Seek only when the mouse is released, like the normal slider.
        void overlaySeek_Committed(System::Object^ sender, System::EventArgs^ e)
        {
            SeekToPercent(fsOverlay->seekBar->Value * 100.0);
            UpdateOverlayPlayback();
        }

        // Volume changes live; going through tbVolume keeps both sliders in step.
        void overlayVolume_Changed(System::Object^ sender, System::EventArgs^ e)
        {
            this->tbVolume->Value = static_cast<int>(Math::Round(fsOverlay->volumeBar->Value * 100.0));
            tbVolume_Scroll(this, System::EventArgs::Empty);
        }

        void overlayExit_Click(System::Object^ sender, System::EventArgs^ e)
        {
            ExitFullScreen();
        }

        // Closing the player while in full screen: remove the overlay first.
        void MpvPlayerForm_FormClosing(System::Object^ sender, FormClosingEventArgs^ e)
        {
            CloseOverlay();
        }

        // Safety check: mpv can't follow a new handle, so warn if panel1's changed.
        void CheckVideoHandle()
        {
            if (!mpv) return;
            if (!this->panel1->IsHandleCreated || this->panel1->Handle != videoHandle) {
                MessageBox::Show(L"The video window handle changed. Video may stop showing until the player is restarted.",
                    L"Full Screen", MessageBoxButtons::OK, MessageBoxIcon::Warning);
            }
        }

        void btnTrack_Click(System::Object^ sender, System::EventArgs^ e)
        {
            if (!mpv) return;
            const char* cmd[] = { "cycle", "audio", NULL };
            mpv_command(mpv, cmd);
        }

        // stereotools mlev lowers the centre signal, where lead vocals usually sit.
        // 0.015625 is its minimum (about -36 dB). The @vocal label lets
        // "af remove @vocal" remove only this filter.
        void btnVocal_Click(System::Object^ sender, System::EventArgs^ e)
        {
            if (!mpv) return;

            int result;
            if (!isVocalReduced) {
                const char* cmd[] = { "af", "add", "@vocal:lavfi=[stereotools=mlev=0.015625]", NULL };
                result = mpv_command(mpv, cmd);
            }
            else {
                const char* cmd[] = { "af", "remove", "@vocal", NULL };
                result = mpv_command(mpv, cmd);
            }

            if (result < 0) {
                // mpv undoes a failed filter change itself, so the state stays as it was.
                String^ action = isVocalReduced ? L"restore normal vocals" : L"reduce vocals";
                MessageBox::Show(
                    L"Could not " + action + L".\n\nmpv error: " +
                    gcnew String(mpv_error_string(result)) + L" (code " + result + L")",
                    L"Vocal filter", MessageBoxButtons::OK, MessageBoxIcon::Warning);
                return;
            }

            isVocalReduced = !isVocalReduced;
            UpdateVocalButton();
        }

        // Green "Vocals OFF" while reduced.
        void UpdateVocalButton()
        {
            if (isVocalReduced) {
                this->btnVocal->Text = L"Vocals OFF";
                this->btnVocal->UseVisualStyleBackColor = false;
                this->btnVocal->BackColor = System::Drawing::Color::LightGreen;
            }
            else {
                this->btnVocal->Text = L"Vocals ON";
                this->btnVocal->ResetBackColor();
                this->btnVocal->UseVisualStyleBackColor = true;
            }
            if (fsOverlay != nullptr)
                fsOverlay->SetToggle(fsOverlay->btnVocals, this->btnVocal->Text, isVocalReduced);
        }
    };
}