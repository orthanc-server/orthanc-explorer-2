import { createStore, createLogger } from 'vuex'
import configuration from './modules/configuration'
import studies from './modules/studies'
import jobs from './modules/jobs'
import labels from './modules/labels'

const debug = process.env.NODE_ENV !== 'production'

export default createStore({
  modules: {
    studies,
    configuration,
    jobs,
    labels
  },
  strict: debug,
  plugins: debug ? [createLogger()] : []
})