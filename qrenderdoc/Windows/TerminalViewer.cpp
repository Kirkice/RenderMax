/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2026 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "TerminalViewer.h"
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include "Code/QRDUtils.h"
#include "Code/Resources.h"
#ifdef WIN32
#include <windows.h>
#include <wrl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "3rdparty/terminal/WebView2.h"
#include "3rdparty/terminal/WebView2EnvironmentOptions.h"
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Callback;
#endif

struct TerminalViewer::Native
{
#ifdef WIN32
  ComPtr<ICoreWebView2Controller> controller;
  ComPtr<ICoreWebView2> web;
  HANDLE input = NULL, output = NULL, process = NULL, job = NULL;
  HANDLE pseudo = NULL;
  typedef HRESULT (WINAPI *CreateFn)(COORD, HANDLE, HANDLE, DWORD, HANDLE *);
  typedef HRESULT (WINAPI *ResizeFn)(HANDLE, COORD);
  typedef void (WINAPI *CloseFn)(HANDLE);
  CreateFn create = NULL;
  ResizeFn resize = NULL;
  CloseFn close = NULL;
  HMODULE loader = NULL;
  std::thread writer;
  std::mutex mutex;
  std::condition_variable wake;
  QByteArray inputQueue;
  bool stopping = false;
#endif
  bool ready = false;
  bool initialNavigation = true;
  int cols = 100, rows = 30;
  int sequence = 0, pending = 0;
};

TerminalViewer::TerminalViewer(QWidget *parent) : QFrame(parent), n(new Native)
{
  setObjectName(lit("terminalViewer"));
  setWindowTitle(tr("Terminal Viewer"));
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(0);
  QFrame *bar = new QFrame(this);
  bar->setObjectName(lit("terminalToolbar"));
  QHBoxLayout *tools = new QHBoxLayout(bar);
  m_Shell = new QComboBox(bar);
  const QString root = QProcessEnvironment::systemEnvironment().value(lit("SystemRoot"), lit("C:/Windows"));
  m_Shell->addItem(tr("Windows PowerShell"), QDir(root).filePath(lit("System32/WindowsPowerShell/v1.0/powershell.exe")));
  const QString pwsh = QStandardPaths::findExecutable(lit("pwsh"));
  if(!pwsh.isEmpty()) m_Shell->addItem(tr("PowerShell 7"), pwsh);
  tools->addWidget(m_Shell);
  m_Status = new QLabel(tr("Loading terminal..."), bar);
  tools->addWidget(m_Status,1);
  QToolButton *restart = new QToolButton(bar);
  restart->setIcon(Icons::arrow_refresh());
  restart->setAutoRaise(true);
  restart->setToolTip(tr("Restart PowerShell"));
  tools->addWidget(restart);
  connect(restart,&QToolButton::clicked,this,&TerminalViewer::restartShell);
  connect(m_Shell,QOverload<int>::of(&QComboBox::activated),this,[this](int){restartShell();});
  layout->addWidget(bar);
  m_Host = new QWidget(this);
  m_Host->setAttribute(Qt::WA_NativeWindow);
  m_Host->installEventFilter(this);
  layout->addWidget(m_Host,1);
  m_Timer = new QTimer(this);
  m_Timer->setInterval(15);
  connect(m_Timer,&QTimer::timeout,this,&TerminalViewer::poll);
  // Dock placement may recreate the native HWND; create WebView only after docking completes.
  QTimer::singleShot(0, this, &TerminalViewer::initialise);
}

TerminalViewer::~TerminalViewer()
{
  stopShell();
#ifdef WIN32
  if(n->controller) n->controller->Close();
  // The loader remains loaded: asynchronous WebView callbacks may still be unwinding.
#endif
  delete n;
}

void TerminalViewer::initialise()
{
#ifdef WIN32
  HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
  n->create = (Native::CreateFn)GetProcAddress(kernel,"CreatePseudoConsole");
  n->resize = (Native::ResizeFn)GetProcAddress(kernel,"ResizePseudoConsole");
  n->close = (Native::CloseFn)GetProcAddress(kernel,"ClosePseudoConsole");
  if(!n->create || !n->resize || !n->close)
  { m_Status->setText(tr("ConPTY requires Windows 10 1809 or later.")); return; }
  const QString loader = QDir(QCoreApplication::applicationDirPath()).filePath(lit("WebView2Loader.dll"));
  n->loader = LoadLibraryW((LPCWSTR)loader.utf16());
  typedef HRESULT (STDAPICALLTYPE *EnvironmentFn)(PCWSTR,PCWSTR,ICoreWebView2EnvironmentOptions *,ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *);
  EnvironmentFn createEnvironment = n->loader ? (EnvironmentFn)GetProcAddress(n->loader,"CreateCoreWebView2EnvironmentWithOptions") : NULL;
  if(!createEnvironment) { m_Status->setText(tr("WebView2Loader.dll is missing.")); return; }
  QPointer<TerminalViewer> self(this);
  const QString cache = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath(lit("TerminalWebView2"));
  // Terminal ANSI RGB values are sRGB, not document colors to transform through
  // the monitor ICC profile. Match native terminal output, including solid fills.
  auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
  options->put_AdditionalBrowserArguments(L"--force-color-profile=srgb");
  HRESULT result = createEnvironment(NULL,(LPCWSTR)cache.utf16(),options.Get(),
    Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([self](HRESULT hr,ICoreWebView2Environment *env)->HRESULT {
      if(!self) return S_OK;
      if(FAILED(hr) || !env) {self->m_Status->setText(tr("WebView2 Runtime is unavailable."));return S_OK;}
      return env->CreateCoreWebView2Controller((HWND)self->m_Host->winId(),
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>([self](HRESULT error,ICoreWebView2Controller *controller)->HRESULT {
          if(!self) {if(controller) controller->Close();return S_OK;}
          if(FAILED(error)||!controller){self->m_Status->setText(tr("Could not create terminal renderer."));return S_OK;}
          self->n->controller=controller;
          controller->get_CoreWebView2(&self->n->web);
          ComPtr<ICoreWebView2Settings> settings;
          self->n->web->get_Settings(&settings);
          settings->put_AreDevToolsEnabled(FALSE);
          settings->put_AreDefaultContextMenusEnabled(FALSE);
          settings->put_IsWebMessageEnabled(TRUE);
          settings->put_IsScriptEnabled(TRUE);
          EventRegistrationToken token;
          self->n->web->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>([self](ICoreWebView2 *,ICoreWebView2WebMessageReceivedEventArgs *args)->HRESULT {
            if(!self)return S_OK;
            LPWSTR json=NULL;args->get_WebMessageAsJson(&json);
            if(json){self->message(QString::fromWCharArray(json));CoTaskMemFree(json);}return S_OK;
          }).Get(),&token);
          self->n->web->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>([self](ICoreWebView2 *,ICoreWebView2NavigationStartingEventArgs *args)->HRESULT {
            LPWSTR uri=NULL;args->get_Uri(&uri);
            if(!self || !self->n->initialNavigation)
              args->put_Cancel(TRUE);
            if(self) self->n->initialNavigation=false;
            CoTaskMemFree(uri);return S_OK;
          }).Get(),&token);
          QFile html(lit(":/terminal/terminal.html")),js(lit(":/terminal/xterm.js")),css(lit(":/terminal/xterm.css"));
          html.open(QIODevice::ReadOnly);js.open(QIODevice::ReadOnly);css.open(QIODevice::ReadOnly);
          QString page=QString::fromUtf8(html.readAll());
          page.replace(lit("/*XTERM_CSS*/"),QString::fromUtf8(css.readAll()));
          page.replace(lit("/*XTERM_JS*/"),QString::fromUtf8(js.readAll()));
          self->resizeRenderer();
          HRESULT navigation = self->n->web->NavigateToString((LPCWSTR)page.utf16());
          if(FAILED(navigation)) self->m_Status->setText(lit("Renderer navigation failed: %1").arg((qulonglong)(unsigned long)navigation));
          return S_OK;
        }).Get());
    }).Get());
  if(FAILED(result))m_Status->setText(tr("Could not initialise WebView2."));
#else
  m_Status->setText(tr("This terminal requires Windows."));
#endif
}

void TerminalViewer::send(const QJsonObject &object)
{
#ifdef WIN32
  if(n->web){const QString json=QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));n->web->PostWebMessageAsJson((LPCWSTR)json.utf16());}
#endif
}

void TerminalViewer::message(const QString &json)
{
  const QJsonObject object=QJsonDocument::fromJson(json.toUtf8()).object();
  const QString type=object.value(lit("type")).toString();
  if(type==lit("error")) m_Status->setText(object.value(lit("data")).toString());
  else if(type==lit("ready")){n->ready=true;restartShell();}
  else if(type==lit("ack")){if(object.value(lit("id")).toInt()==n->pending)n->pending=0;}
  else if(type==lit("copy"))QApplication::clipboard()->setText(object.value(lit("data")).toString());
  else if(type==lit("paste"))send(QJsonObject{{lit("type"),lit("paste")},{lit("data"),QApplication::clipboard()->text()}});
#ifdef WIN32
  else if(type==lit("resize")){
    n->cols=qBound(2,object.value(lit("cols")).toInt(),500);
    n->rows=qBound(1,object.value(lit("rows")).toInt(),300);
    if(n->pseudo)n->resize(n->pseudo,COORD{(SHORT)n->cols,(SHORT)n->rows});
  }
  else if(type==lit("input") && n->input){
    const QByteArray bytes=object.value(lit("data")).toString().toUtf8();
    std::lock_guard<std::mutex> lock(n->mutex);
    if(n->inputQueue.size()+bytes.size()<=1024*1024)
    { n->inputQueue+=bytes; n->wake.notify_one(); }
    else m_Status->setText(tr("Input queue full; paste rejected."));
  }
#endif
}

void TerminalViewer::restartShell()
{
  if(!n->ready)return;
  stopShell();
  send(QJsonObject{{lit("type"),lit("reset")}});
#ifdef WIN32
  HANDLE childInput=NULL,childOutput=NULL;
  if(!CreatePipe(&childInput,&n->input,NULL,0)||!CreatePipe(&n->output,&childOutput,NULL,0)){
    if(childInput)CloseHandle(childInput);if(childOutput)CloseHandle(childOutput);stopShell();m_Status->setText(tr("Could not create terminal pipes."));return;}
  HRESULT hr=n->create(COORD{(SHORT)n->cols,(SHORT)n->rows},childInput,childOutput,0,&n->pseudo);
  CloseHandle(childInput);CloseHandle(childOutput);
  if(FAILED(hr)){stopShell();m_Status->setText(tr("Could not create ConPTY."));return;}
  SIZE_T size=0;InitializeProcThreadAttributeList(NULL,1,0,&size);
  QByteArray storage((int)size,0);
  LPPROC_THREAD_ATTRIBUTE_LIST attributes=(LPPROC_THREAD_ATTRIBUTE_LIST)storage.data();
  bool attributesReady=InitializeProcThreadAttributeList(attributes,1,0,&size)!=FALSE;
  const DWORD_PTR pseudoAttribute=0x00020016;
  if(!attributesReady || !UpdateProcThreadAttribute(attributes,0,pseudoAttribute,n->pseudo,sizeof(HANDLE),NULL,NULL)){
    if(attributesReady)DeleteProcThreadAttributeList(attributes);stopShell();m_Status->setText(tr("Could not configure ConPTY."));return;}
  STARTUPINFOEXW info={};info.StartupInfo.cb=sizeof(info);info.lpAttributeList=attributes;
  const QString program=QDir::toNativeSeparators(m_Shell->currentData().toString());
  std::wstring command=(lit("\"")+program+lit("\" -NoLogo -NoProfile")).toStdWString();
  std::wstring directory=QDir::toNativeSeparators(QDir::currentPath()).toStdWString();
  PROCESS_INFORMATION process={};
  n->job=CreateJobObjectW(NULL,NULL);
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits={};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  bool ok=n->job && SetInformationJobObject(n->job,JobObjectExtendedLimitInformation,&limits,sizeof(limits)) &&
    CreateProcessW((LPCWSTR)program.utf16(),&command[0],NULL,NULL,FALSE,EXTENDED_STARTUPINFO_PRESENT|CREATE_SUSPENDED,NULL,directory.c_str(),&info.StartupInfo,&process);
  DeleteProcThreadAttributeList(attributes);
  if(ok){n->process=process.hProcess;ok=AssignProcessToJobObject(n->job,process.hProcess) && ResumeThread(process.hThread)!=(DWORD)-1;CloseHandle(process.hThread);}
  if(!ok){stopShell();m_Status->setText(tr("Could not start PowerShell in ConPTY."));return;}
  n->stopping=false;
  n->writer=std::thread([this]() {
    for(;;)
    {
      QByteArray bytes;
      {
        std::unique_lock<std::mutex> lock(n->mutex);
        n->wake.wait(lock,[this](){return n->stopping || !n->inputQueue.isEmpty();});
        if(n->stopping) return;
        bytes.swap(n->inputQueue);
      }
      int offset=0;
      while(offset<bytes.size())
      {
        {std::lock_guard<std::mutex> lock(n->mutex);if(n->stopping)return;}
        DWORD written=0;
        if(!WriteFile(n->input,bytes.constData()+offset,bytes.size()-offset,&written,NULL) || !written)return;
        offset+=written;
      }
    }
  });
  m_Status->setText(tr("PowerShell - ConPTY"));m_Timer->start();
#endif
}

void TerminalViewer::poll()
{
#ifdef WIN32
  if(n->output && !n->pending){DWORD available=0;if(PeekNamedPipe(n->output,NULL,0,NULL,&available,NULL)&&available){
    QByteArray bytes(qMin((int)available,32768),0);DWORD read=0;
    if(ReadFile(n->output,bytes.data(),bytes.size(),&read,NULL)&&read){bytes.resize(read);n->pending=++n->sequence;
      send(QJsonObject{{lit("type"),lit("output")},{lit("data"),QString::fromLatin1(bytes.toBase64())},{lit("id"),n->pending}});}
  }}
  if(n->process && WaitForSingleObject(n->process,0)!=WAIT_TIMEOUT){m_Timer->stop();m_Status->setText(tr("PowerShell exited. Use Restart."));}
#endif
}

void TerminalViewer::stopShell()
{
  m_Timer->stop();n->pending=0;
#ifdef WIN32
  if(n->job){TerminateJobObject(n->job,0);CloseHandle(n->job);n->job=NULL;}
  if(n->process){TerminateProcess(n->process,0);CloseHandle(n->process);n->process=NULL;}
  {
    std::lock_guard<std::mutex> lock(n->mutex);
    n->stopping=true;
    n->inputQueue.clear();
  }
  n->wake.notify_one();
  if(n->writer.joinable())
  {
    HANDLE thread=(HANDLE)n->writer.native_handle();
    while(WaitForSingleObject(thread,10)==WAIT_TIMEOUT)
      CancelSynchronousIo(thread);
    n->writer.join();
  }
  if(n->input){CloseHandle(n->input);n->input=NULL;}
  // Close the read endpoint first so ConPTY cannot block trying to flush to an undrained pipe.
  if(n->output){CloseHandle(n->output);n->output=NULL;}
  if(n->pseudo){n->close(n->pseudo);n->pseudo=NULL;}
#endif
}

void TerminalViewer::resizeRenderer()
{
#ifdef WIN32
  if(n->controller){RECT rect={};GetClientRect((HWND)m_Host->winId(),&rect);n->controller->put_Bounds(rect);}
#endif
}

bool TerminalViewer::eventFilter(QObject *watched,QEvent *event)
{
  if(watched==m_Host && event->type()==QEvent::Resize)resizeRenderer();
  return QFrame::eventFilter(watched,event);
}
void TerminalViewer::showEvent(QShowEvent *event)
{
  QFrame::showEvent(event);resizeRenderer();send(QJsonObject{{lit("type"),lit("focus")}});
}
