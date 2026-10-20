#include "MpvPlayerForm.h"

using namespace System;
using namespace System::Windows::Forms;

// Windows Forms needs a single-threaded apartment (STA).
[STAThread]
int main(array<String^>^ args)
{
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);

    MyMediaPlayer::MpvPlayerForm^ form = gcnew MyMediaPlayer::MpvPlayerForm();
    Application::Run(form);

    return 0;
}