/* -*- Mode: Java; c-basic-offset: 4; tab-width: 20; indent-tabs-mode: nil; -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.ContentResolver;
import android.os.SystemClock;
import android.util.Log;

public class Tabs implements GeckoEventListener {
    private static final String LOGTAG = "GeckoTabs";

    private Tab selectedTab;
    private HashMap<Integer, Tab> tabs;
    private ArrayList<Tab> order;
    private ContentResolver resolver;
    private boolean mRestoringSession = false;

    private Tabs() {
        tabs = new HashMap<Integer, Tab>();
        order = new ArrayList<Tab>();
        GeckoAppShell.registerGeckoEventListener("SessionHistory:New", this);
        GeckoAppShell.registerGeckoEventListener("SessionHistory:Back", this);
        GeckoAppShell.registerGeckoEventListener("SessionHistory:Forward", this);
        GeckoAppShell.registerGeckoEventListener("SessionHistory:Goto", this);
        GeckoAppShell.registerGeckoEventListener("SessionHistory:Purge", this);
        GeckoAppShell.registerGeckoEventListener("Tab:Added", this);
        GeckoAppShell.registerGeckoEventListener("Tab:Close", this);
        GeckoAppShell.registerGeckoEventListener("Tab:Select", this);
        GeckoAppShell.registerGeckoEventListener("Session:RestoreBegin", this);
        GeckoAppShell.registerGeckoEventListener("Session:RestoreEnd", this);
    }

    public int getCount() {
        return tabs.size();
    }

    public Tab addTab(JSONObject params) throws JSONException {
        int id = params.getInt("tabID");
        if (tabs.containsKey(id))
           return tabs.get(id);

        // null strings return "null" (http://code.google.com/p/android/issues/detail?id=13830)
        String url = params.isNull("uri") ? null : params.getString("uri");
        Boolean external = params.getBoolean("external");
        int parentId = params.getInt("parentId");
        String title = params.getString("title");

        Tab tab = new Tab(id, url, external, parentId, title);
        tabs.put(id, tab);
        order.add(tab);

        if (!mRestoringSession) {
            GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                public void run() {
                    GeckoApp.mBrowserToolbar.updateTabCountAndAnimate(getCount());
                    GeckoApp.mBrowserToolbar.updateBackButton(false);
                    GeckoApp.mBrowserToolbar.updateForwardButton(false);
                    GeckoApp.mAppContext.invalidateOptionsMenu();
                }
            });
        }

        Log.i(LOGTAG, "Added a tab with id: " + id);
        return tab;
    }

    public void removeTab(int id) {
        if (tabs.containsKey(id)) {
            order.remove(getTab(id));
            tabs.remove(id);
            Log.i(LOGTAG, "Removed a tab with id: " + id);
        }
    }

    public Tab selectTab(int id) {
        if (!tabs.containsKey(id))
            return null;

        final Tab oldTab = getSelectedTab();
        final Tab tab = tabs.get(id);
        // This avoids a NPE below, but callers need to be careful to
        // handle this case
        if (tab == null)
            return null;

        if ("about:home".equals(tab.getURL()))
            GeckoApp.mAppContext.showAboutHome();
        else
            GeckoApp.mAppContext.hideAboutHome();

        selectedTab = tab;
        GeckoApp.mAppContext.mMainHandler.post(new Runnable() { 
            public void run() {
                GeckoApp.mFormAssistPopup.hide();
                if (isSelectedTab(tab)) {
                    String url = tab.getURL();
                    GeckoApp.mBrowserToolbar.refresh();
                    GeckoApp.mAppContext.invalidateOptionsMenu();
                    GeckoApp.mDoorHangerPopup.updatePopup();
                    notifyListeners(tab, TabEvents.SELECTED);

                    if (oldTab != null)
                        GeckoApp.mAppContext.hidePlugins(oldTab);
                }
            }
        });

        // Pass a message to Gecko to update tab state in BrowserApp
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Selected", String.valueOf(tab.getId())));
        return tab;
    }

    public int getIndexOf(Tab tab) {
        return order.lastIndexOf(tab);
    }

    public Tab getTabAt(int index) {
        if (index >= 0 && index < order.size())
            return order.get(index);
        else
            return null;
    }

    public Tab getSelectedTab() {
        return selectedTab;
    }

    public boolean isSelectedTab(Tab tab) {
        if (selectedTab == null)
            return false;

        return tab == selectedTab;
    }

    public Tab getTab(int id) {
        if (getCount() == 0)
            return null;

        if (!tabs.containsKey(id))
           return null;

        return tabs.get(id);
    }

    /** Close tab and then select the default next tab */
    public void closeTab(Tab tab) {
        closeTab(tab, getNextTab(tab));
    }

    /** Close tab and then select nextTab */
    public void closeTab(final Tab tab, Tab nextTab) {
        if (tab == null || nextTab == null)
            return;

        selectTab(nextTab.getId());

        int tabId = tab.getId();
        removeTab(tabId);

        GeckoApp.mAppContext.mMainHandler.post(new Runnable() { 
            public void run() {
                notifyListeners(tab, TabEvents.CLOSED);
                GeckoApp.mBrowserToolbar.updateTabCountAndAnimate(Tabs.getInstance().getCount());
                GeckoApp.mDoorHangerPopup.updatePopup();
                GeckoApp.mAppContext.hidePlugins(tab);
                tab.onDestroy();
            }
        });

        // Pass a message to Gecko to update tab state in BrowserApp
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Closed", String.valueOf(tabId)));
    }

    /** Return the tab that will be selected by default after this one is closed */
    public Tab getNextTab(Tab tab) {
        Tab selectedTab = getSelectedTab();
        if (selectedTab != tab)
            return selectedTab;

        int index = getIndexOf(tab);
        Tab nextTab = getTabAt(index + 1);
        if (nextTab == null)
            nextTab = getTabAt(index - 1);

        Tab parent = getTab(tab.getParentId());
        if (parent != null) {
            // If the next tab is a sibling, switch to it. Otherwise go back to the parent.
            if (nextTab != null && nextTab.getParentId() == tab.getParentId())
                return nextTab;
            else
                return parent;
        }
        return nextTab;
    }

    public HashMap<Integer, Tab> getTabs() {
        if (getCount() == 0)
            return null;

        return tabs;
    }
    
    public ArrayList<Tab> getTabsInOrder() {
        if (getCount() == 0)
            return null;

        return order;
    }

    public void setContentResolver(ContentResolver resolver) {
        this.resolver = resolver;
    }

    public ContentResolver getContentResolver() {
        return resolver;
    }

    //Making Tabs a singleton class
    private static class TabsInstanceHolder {
        private static final Tabs INSTANCE = new Tabs();
    }

    public static Tabs getInstance() {
       return Tabs.TabsInstanceHolder.INSTANCE;
    }

    // GeckoEventListener implementation

    public void handleMessage(String event, JSONObject message) {
        Log.i(LOGTAG, "Got message: " + event);
        try {
            if (event.startsWith("SessionHistory:")) {
                Tab tab = getTab(message.getInt("tabID"));
                if (tab != null) {
                    event = event.substring("SessionHistory:".length());
                    tab.handleSessionHistoryMessage(event, message);
                }
            } else if (event.equals("Tab:Added")) {
                Log.i(LOGTAG, "Received message from Gecko: " + SystemClock.uptimeMillis() + " - Tab:Added");
                Tab tab = addTab(message);
                if (message.getBoolean("selected"))
                    selectTab(tab.getId());
                if (message.getBoolean("delayLoad"))
                    tab.setState(Tab.STATE_DELAYED);
            } else if (event.equals("Tab:Close")) {
                Tab tab = getTab(message.getInt("tabID"));
                closeTab(tab);
            } else if (event.equals("Tab:Select")) {
                selectTab(message.getInt("tabID"));
            } else if (event.equals("Session:RestoreBegin")) {
                mRestoringSession = true;
            } else if (event.equals("Session:RestoreEnd")) {
                mRestoringSession = false;
                GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                    public void run() {
                        GeckoApp.mBrowserToolbar.refresh();
                    }
                });
            }
        } catch (Exception e) { 
            Log.i(LOGTAG, "handleMessage throws " + e + " for message: " + event);
        }
    }

    public void refreshThumbnails() {
        Iterator<Tab> iterator = tabs.values().iterator();
        while (iterator.hasNext()) {
            final Tab tab = iterator.next();
            GeckoAppShell.getHandler().post(new Runnable() {
                public void run() {
                    GeckoApp.mAppContext.getAndProcessThumbnailForTab(tab);
                }
            });
        }
    }

    public interface OnTabsChangedListener {
        public void onTabChanged(Tab tab, TabEvents msg);
    }
    
    private static ArrayList<OnTabsChangedListener> mTabsChangedListeners;

    public static void registerOnTabsChangedListener(OnTabsChangedListener listener) {
        if (mTabsChangedListeners == null)
            mTabsChangedListeners = new ArrayList<OnTabsChangedListener>();
        
        mTabsChangedListeners.add(listener);
    }

    public static void unregisterOnTabsChangedListener(OnTabsChangedListener listener) {
        if (mTabsChangedListeners == null)
            return;
        
        mTabsChangedListeners.remove(listener);
    }

    public enum TabEvents {
        CLOSED,
        START,
        LOADED,
        STOP,
        FAVICON,
        THUMBNAIL,
        TITLE,
        SELECTED
    }

    public void notifyListeners(Tab tab, TabEvents msg) {
        if (mTabsChangedListeners == null)
            return;

        Iterator<OnTabsChangedListener> items = mTabsChangedListeners.iterator();
        while (items.hasNext()) {
            items.next().onTabChanged(tab, msg);
        }
    }
}
