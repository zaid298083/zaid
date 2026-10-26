#pragma once

class SplashScreen : public Widget
{
public:
	SplashScreen();

	virtual void performLayout() override;
	virtual void draw(ImGuiRenderer* renderer) override;

	// Loading status methods (called from C++ to update Java layer)
	void setLoadStatus(const char* status);
	void setMusicPlaylist(const char* tracks[], int trackCount, const char* title);
	void stopMusic();

private:
	Image* m_logo;
	ProgressBar* m_progressBar;
};