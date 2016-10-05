function plot3dSkeletonNoHandFoot(skeletonData, plotColor)
    index = [3, 4; 3, 21; 21, 2; 2, 1; 21, 9; 21, 5; 1, 17; 1, 13; ...
        9, 10; 10, 11; 11, 12; 5, 6; 6, 7; 7, 8; ...
        17, 18; 18, 19; 13, 14; 14, 15];
    
    for i = 1:size(index, 1)
        plot3(skeletonData(index(i, :), 1), skeletonData(index(i, :), 2), skeletonData(index(i, :), 3), ...
            'Color', plotColor, 'LineWidth', 3);
    end
end