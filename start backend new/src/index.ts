import express, { Express } from 'express'
import cors from 'cors'
import helmet from 'helmet'
import * as middleware from './middleware'
import articlesRouter from './routers/articles.router'
import authRouter from './routers/auth.router'
import { connectDB } from './db/connect'
import dotenv from 'dotenv'
import mongoose from 'mongoose'
dotenv.config()

const ENV = process.env.NODE_ENV || 'production'
const PORT: string | number = process.env.PORT || 5500
const MONGO_URI: string = process.env.MONGO_URI || ''
const app: Express = express()

app.use(helmet())
app.use(cors())
app.use(express.json())
app.use(middleware.httpLogger)
app.get('/api/v1', (req, res) => {
  console.log('Home GET Request')
  res.status(200).json({ ms: 'Home GET Request', port: `${PORT}` })
})
app.use('/api/v1/articles', articlesRouter)
app.use('/api/v1/auth', authRouter)
app.use(middleware.errorHandler)
app.use(middleware.notFoundHandler)

const server = async () => {
  try {
    await connectDB(MONGO_URI)
    app.listen(PORT, () =>
      console.log(
        `Server is listening on PORT ${PORT}... on ${ENV} environment`
      )
    )
  } catch (error) {
    console.log(error)
  }
}

server()

export { app as default, server }
