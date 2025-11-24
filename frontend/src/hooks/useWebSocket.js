import { useEffect, useState, useRef } from "react";

export const useWebSocket = (url) => {
    const [data, setData] = useState(null);
    const wsRef = useRef(null);

    useEffect(() => {
        wsRef.current = new WebSocket(url);

        wsRef.current.onopen = () => {
            console.log("WebSocket connected!");
        };

        wsRef.current.onmessage = (event) => {
            // Handle both binary and text
            let value = 69;
            if (event.data instanceof Blob) {
                const reader = new FileReader();
                reader.onload = () => {
                    const arrayBuffer = reader.result;
                    const view = new DataView(arrayBuffer);
                    value = view.getInt32(0, true); // true = little-endian
                    setData(value);
                };
                reader.readAsArrayBuffer(event.data);
            } else {
                // Text
                value = event.data;
                console.log(value);
                setData(value);
            }
        };

        wsRef.current.onclose = () => {
            console.log("WebSocket disconnected!");
        };

        return () => wsRef.current.close();
    }, [url]);

    return data;
};
