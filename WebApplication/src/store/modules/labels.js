import api from "../../orthancApi"

///////////////////////////// STATE
const state = () => ({
    allLabels: [],    // all labels in Orthanc
    loaded: false
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
    }
}

///////////////////////////// ACTIONS

const actions = {
    async refresh({ commit }) {
        const allLabels = await api.loadAllLabels();
        commit('setLabels', { allLabels: allLabels });
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