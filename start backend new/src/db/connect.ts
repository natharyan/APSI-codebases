import mongoose from 'mongoose'
// import { MongoClient, ServerApiVersion } from 'mongodb'

// Create a MongoClient with a MongoClientOptions object to set the Stable API version


const connectDB = async (url: string) => {
  // const client = new MongoClient(url, {
  //   serverApi: {
  //     version: ServerApiVersion.v1,
  //     strict: true,
  //     deprecationErrors: true,
  //   }
  // })
  // // mongoose.connect(url)
  // //   .then(() => console.log('MongoDB connected'))
  // //   .catch((err) => console.error('MongoDB connection error:', err))

  // try {
  //   // Connect the client to the server	(optional starting in v4.7)
  //   await client.connect()
  //   // Send a ping to confirm a successful connection
  //   await client.db('admin').command({ ping: 1 })
  //   console.log('Pinged your deployment. You successfully connected to MongoDB!')
  // } finally {
  //   // Ensures that the client will close when you finish/error
  //   await client.close()
  // }
  mongoose.connect(url)
    .then(() => console.log('Database Connected'))
    .catch((err) => console.error('MongoDB Connection Failed:', err))
}
mongoose.set('debug', true)

export { connectDB }
