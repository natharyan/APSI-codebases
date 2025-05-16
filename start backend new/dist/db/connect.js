"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.connectDB = void 0;
const mongoose_1 = __importDefault(require("mongoose"));
// import { MongoClient, ServerApiVersion } from 'mongodb'
// Create a MongoClient with a MongoClientOptions object to set the Stable API version
const connectDB = async (url) => {
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
    mongoose_1.default.connect(url)
        .then(() => console.log('Database Connected'))
        .catch((err) => console.error('MongoDB Connection Failed:', err));
};
exports.connectDB = connectDB;
mongoose_1.default.set('debug', true);
