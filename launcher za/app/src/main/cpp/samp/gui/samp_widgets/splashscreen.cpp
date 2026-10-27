#include "../gui.h"
#include "../../main.h"
#include "../../java/jniutil.h"

extern CJavaWrapper* pJavaWrapper;

SplashScreen::SplashScreen()
{
	// The Java LoadingScreen handles the visual loading screen now.
	// This C++ SplashScreen is kept minimal for compatibility.
	int x = rand() % 2;
	switch (x)
	{
		case 0:
			m_logo = new Image("menu", "menu_adjback");
			break;

		case 1:
			m_logo = new Image("menu", "menu_adjback2");
			break;
	}

	//----------- //
	//m_logo = new Image("samp", "mylogo");
	//m_logo = new Image("fm", "mylogo");
	this->addChild(m_logo);

	m_progressBar = new ProgressBar(ImColor(0.0f, 0.0f, 0.0f),
		ImColor(190, 0, 0), ImColor(255, 100, 0),
		ImColor(255, 100, 0), ImColor(190, 0, 0));
	this->addChild(m_progressBar);
}

void SplashScreen::performLayout()
{
	m_logo->setFixedSize(UISettings::splashscreenLogoSize());
	m_logo->setPosition(UISettings::splashscreenLogoPos());

	m_progressBar->setFixedSize(UISettings::splashscreenPBarSize());
	m_progressBar->setPosition(UISettings::splashscreenPBarPos());
}

void SplashScreen::draw(ImGuiRenderer* renderer)
{
	// background
	renderer->drawRect(absolutePosition(), absolutePosition() + size(), ImColor(0.0f, 0.0f, 0.0f), true);
	// logo
	m_logo->draw(renderer);
	// progress bar
	m_progressBar->draw(renderer);
}

void SplashScreen::setLoadStatus(const char* status)
{
	if (pJavaWrapper) {
		pJavaWrapper->UpdateLoadingStatus(status);
	}
}

void SplashScreen::setMusicPlaylist(const char* tracks[], int trackCount, const char* title)
{
	if (pJavaWrapper) {
		pJavaWrapper->SetMusicPlaylist(tracks, trackCount, title);
	}
}

void SplashScreen::stopMusic()
{
	if (pJavaWrapper) {
		pJavaWrapper->StopLoadingMusic();
	}
}