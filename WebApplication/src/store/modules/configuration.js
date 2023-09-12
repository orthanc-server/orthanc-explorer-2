import api from "../../orthancApi"

///////////////////////////// STATE
const state = () => ({
    installedPlugins: {},
    orthancPeers: [],
    targetDicomWebServers: [],
    queryableDicomWebServers: [],
    queryableDicomModalities: [],
    targetDicomModalities: [],
    maxStudiesDisplayed: 100,
    orthancApiUrl: "../../",
    uiOptions: {},
    userProfile: null,
    tokens: {},
    loaded: false,
    system: {},
    ohifDataSource: "dicom-web"
})

///////////////////////////// GETTERS
const getters = {
}

///////////////////////////// MUTATIONS

const mutations = {
    setUiOptions(state, { uiOptions }) {
        state.uiOptions = uiOptions;
    },
    setUserProfile(state, { profile }) {
        state.userProfile = profile;
    },
    setInstalledPlugin(state, { plugin, pluginConfiguration }) {
        state.installedPlugins[plugin] = pluginConfiguration;
    },
    setDicomWebServers(state, { dicomWebServers }) {  // TODO: split in two
        state.targetDicomWebServers = dicomWebServers;
        state.queryableDicomWebServers = dicomWebServers;
    },
    setOhifDataSource(state, { ohifDataSource }) {
        state.ohifDataSource = ohifDataSource;
    },
    setOrthancPeers(state, { orthancPeers }) {
        state.orthancPeers = orthancPeers;
    },
    setDicomModalities(state, { dicomModalities }) {  // TODO: split in two
        state.queryableDicomModalities = dicomModalities;
        state.targetDicomModalities = dicomModalities;
    },
    setSystem(state, { system }) {
        state.system = system;
    },
    setLoaded(state) {
        state.loaded = true;
    },
    setTokens(state, { tokens }) {
        state.tokens = tokens;
    }

}

///////////////////////////// ACTIONS

const actions = {
    async load({ commit, state}) {
        await this.dispatch('configuration/loadOe2Configuration');

        if (state.uiOptions.EnableSendTo) {
            try {
                const orthancPeers = await api.loadOrthancPeers();
                commit('setOrthancPeers', { orthancPeers: orthancPeers});
            } catch (err) {
                console.warn("can not get Orthanc peers - not authorized ?")
            }
        }

        if (state.uiOptions.EnableSendTo || state.uiOptions.EnableDicomModalities) {
            try {
                const dicomModalities = await api.loadDicomModalities();
                commit('setDicomModalities', { dicomModalities: dicomModalities});
            } catch (err) {
                console.warn("can not get DICOM modalities - not authorized ?")
            }
        }

        const system = await api.loadSystem();
        commit('setSystem', { system: system});

        commit('setLoaded');
    },
    async loadOe2Configuration({ commit }) {
        const oe2Config = await api.loadOe2Configuration();
        commit('setUiOptions', { uiOptions: oe2Config['UiOptions']});
        commit('setTokens', { tokens: oe2Config['Tokens']});

        if ('Profile' in oe2Config) {
            commit('setUserProfile', { profile: oe2Config['Profile']});
        }
        document._mustTranslateDicomTags = oe2Config['UiOptions']['TranslateDicomTags'];

        for (const [pluginName, pluginConfiguration] of Object.entries(oe2Config['Plugins'])) {

            commit('setInstalledPlugin', { plugin: pluginName, pluginConfiguration: pluginConfiguration})

            if (pluginName === "dicom-web") {
                try {
                    const dicomWebServers = await api.loadDicomWebServers();
                    commit('setDicomWebServers', { dicomWebServers: dicomWebServers});
                } catch (err) {
                    console.warn("can not get DicomWEB servers - not authorized ?")
                }
            } else if (pluginName === "ohif") {
                commit('setOhifDataSource', { ohifDataSource: pluginConfiguration["DataSource"]})
            }
            
        }

    },
}



///////////////////////////// EXPORT

export default {
    namespaced: true,
    state,
    getters,
    mutations,
    actions,
}