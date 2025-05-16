"use strict";
// import { model, Schema } from 'mongoose'
Object.defineProperty(exports, "__esModule", { value: true });
// const UserSchema: Schema = new Schema(
//     {
//         name: { type: String, required: true },
//         email: { type: String, required: true, unique: true },
//         password: { type: String, required: true },
//         quote: { type: String },
//     },
//     {
//         collection: 'user-data', timestamps: true,
//         useFindAndModify: false
//     },
// )
// export default model('userModels', UserSchema)
const mongoose_1 = require("mongoose");
// Define the User schema
const UserSchema = new mongoose_1.Schema({
    name: { type: String, required: true },
    email: { type: String, required: true, unique: true },
    password: { type: String, required: true },
    quote: { type: String },
}, {
    collection: 'bhishma-cluster',
    timestamps: true,
});
// Create and export the User model
const User = (0, mongoose_1.model)('User', UserSchema);
exports.default = User;
