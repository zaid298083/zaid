package com.nexion.launcher.update;

public class UpdateInfo {
    private boolean update;
    private String version;
    private String update_url;

    public boolean isUpdate() {
        return update;
    }

    public void setUpdate(boolean update) {
        this.update = update;
    }

    public String getVersion() {
        return version;
    }

    public void setVersion(String version) {
        this.version = version;
    }

    public String getUpdateUrl() {
        return update_url;
    }

    public void setUpdateUrl(String update_url) {
        this.update_url = update_url;
    }
}