import { createStore, createLogger } from 'vuex'
import configuration from './modules/configuration'
import studies from './modules/studies'
import selection from './modules/selection'
import jobs from './modules/jobs'
import labels from './modules/labels'

const debug = process.env.NODE_ENV !== 'production'

export default createStore({
  modules: {
    studies,
    selection,
    configuration,
    jobs,
    labels
  },
  strict: debug,
  plugins: debug ? [createLogger()] : []
})