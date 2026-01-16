import api from "../../orthancApi"
import store from "../../store"


///////////////////////////// STATE
const state = () => ({
    allLabels: [],    // all labels in Orthanc
    loaded: false,
    canShowStudiesWithoutLabels: false
})

///////////////////////////// GETTERS
const getters = {
}

///////////////////////////// MUTATIONS

const mutations = {
    setLabels(state, { allLabels }) {
        state.allLabels = allLabels;
    },
    setLoaded(state) {
        state.loaded = true;
    },
    setCanShowStudiesWithoutLabels(state) {
        state.canShowStudiesWithoutLabels = true;
    }
}

///////////////////////////// ACTIONS

const actions = {
    async refresh({ commit }) {
        const allLabels = await api.loadAllLabels();
        commit('setLabels', { allLabels: allLabels });

        let showStudiesWithoutLabels = false;
        if (store.state.configuration.system.ApiVersion >= 30 && allLabels.length > 0)
        {
            if (store.state.configuration.userProfile) { // only the users who have access to all labels can see this option !
                showStudiesWithoutLabels = store.state.configuration.userProfile["authorized-labels"].length == 1 && store.state.configuration.userProfile["authorized-labels"][0] == "*";
            } else {
                showStudiesWithoutLabels = true;
            }
            if (showStudiesWithoutLabels) {
                commit('setCanShowStudiesWithoutLabels');
            }
        }

        commit('setLoaded');
    }
}



///////////////////////////// EXPORT

export default {
    namespaced: true,
    state,
    getters,
    mutations,
    actions,
}