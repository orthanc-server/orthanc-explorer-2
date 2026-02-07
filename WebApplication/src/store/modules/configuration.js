import api from "../../orthancApi"
import resourceHelpers from "../../helpers/resource-helpers"

///////////////////////////// STATE
const state = () => ({
    installedPlugins: {},
    orthancPeers: [],
    targetDicomWebServers: [],
    queryableDicomWebServers: [],
    queryableDicomModalities: {},
    targetDicomModalities: {},
    maxStudiesDisplayed: 100,
    orthancApiUrl: "../../",
    uiOptions: {},
    userProfile: null,
    tokens: {},
    loaded: false,
    system: {},
    ohifDataSource: "dicom-web",
    customLogoUrl: null,
    hasCustomLogo: false, 
    requestedTagsForStudyList: [],
    hasExtendedFind: false,
    hasExtendedChanges: false,
    advancedOptions: {}
})

///////////////////////////// GETTERS
const getters = {
}

///////////////////////////// MUTATIONS

const mutations = {
    setUiOptions(state, { uiOptions }) {
        state.uiOptions = uiOptions;

        if (uiOptions.StudyListColumns.indexOf('modalities') != -1) {
            state.requestedTagsForStudyList.push('ModalitiesInStudy')
        }
        if (uiOptions.StudyListColumns.indexOf('instancesCount') != -1 || uiOptions.StudyListColumns.indexOf('seriesAndInstancesCount') != -1) {
            state.requestedTagsForStudyList.push('NumberOfStudyRelatedInstances')
        }
        if (uiOptions.StudyListColumns.indexOf('seriesCount') != -1 || uiOptions.StudyListColumns.indexOf('seriesAndInstancesCount') != -1) {
            state.requestedTagsForStudyList.push('NumberOfStudyRelatedSeries')
        }
        if (uiOptions.EnableReportQuickButton) {
            state.requestedTagsForStudyList.push('SOPClassesInStudy'); // to detect PDF files
        }
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
        state.hasExtendedFind = "Capabilities" in state.system && state.system.Capabilities.HasExtendedFind;
        state.hasExtendedChanges = "Capabilities" in state.system && state.system.Capabilities.HasExtendedChanges;
    },
    setLoaded(state) {
        state.loaded = true;
    },
    setTokens(state, { tokens }) {
        state.tokens = tokens;
    },
    setCustomLogo(state, { customLogoUrl, hasCustomLogo }) {
        state.hasCustomLogo = hasCustomLogo;
        state.customLogoUrl = customLogoUrl;
    },
    setAdvancedOptions(state, { advancedOptions }) {
        state.advancedOptions = advancedOptions;
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
        commit('setAdvancedOptions', { advancedOptions: oe2Config['AdvancedOptions']});

        if ('Profile' in oe2Config) {
            commit('setUserProfile', { profile: oe2Config['Profile']});
        }
        document._mustTranslateDicomTags = oe2Config['UiOptions']['TranslateDicomTags'];

        if ('HasCustomLogo' in oe2Config) {
            let customLogoUrl = null;
            if ('CustomLogoUrl' in oe2Config) {
                customLogoUrl = oe2Config['CustomLogoUrl']
            }
            commit('setCustomLogo', { customLogoUrl: customLogoUrl, hasCustomLogo: oe2Config['HasCustomLogo']});
        }

        if ('CustomTitle' in oe2Config) {
            document.title = oe2Config['CustomTitle'];
        } else {
            document.title = "D.PACS Explorer";
        }

        if ('UiOptions' in oe2Config && 'PatientNameFormatting' in oe2Config['UiOptions'] && 'PatientNameCapture' in oe2Config['UiOptions']) {
            resourceHelpers.patientNameCapture = oe2Config['UiOptions']['PatientNameCapture'];
            resourceHelpers.patientNameFormatting = oe2Config['UiOptions']['PatientNameFormatting'];
        }

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